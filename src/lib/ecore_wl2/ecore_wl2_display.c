#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "ecore_wl2_private.h"

/* for MIN() */
#include <sys/param.h>

static Eina_Bool _fatal_error = EINA_FALSE;
static Eina_Hash *_server_displays = NULL;
static Eina_Hash *_client_displays = NULL;

static void
_ecore_wl2_display_signal_exit(void)
{
   Ecore_Event_Signal_Exit *ev;

   ev = calloc(1, sizeof(Ecore_Event_Signal_Exit));
   if (!ev) return;

   ev->quit = EINA_TRUE;
   ecore_event_add(ECORE_EVENT_SIGNAL_EXIT, ev, NULL, NULL);
}

static void
_xdg_shell_cb_ping(void *data EINA_UNUSED, struct xdg_shell *shell, uint32_t serial)
{
   xdg_shell_pong(shell, serial);
}

static const struct xdg_shell_listener _xdg_shell_listener =
{
   _xdg_shell_cb_ping
};

static void
_cb_global_event_free(void *data EINA_UNUSED, void *event)
{
   Ecore_Wl2_Event_Global *ev;

   ev = event;
   eina_stringshare_del(ev->interface);
   free(ev);
}

static void
_cb_global_add(void *data, struct wl_registry *registry, unsigned int id, const char *interface, unsigned int version)
{
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Event_Global *ev;

   ewd = data;

   /* test to see if we have already added this global to our hash */
   if (!eina_hash_find(ewd->globals, &id))
     {
        Ecore_Wl2_Global *global;

        /* allocate space for new global */
        global = calloc(1, sizeof(Ecore_Wl2_Global));
        if (!global) return;

        global->id = id;
        global->interface = eina_stringshare_add(interface);
        global->version = version;

        /* add this global to our hash */
        if (!eina_hash_add(ewd->globals, &global->id, global))
          {
             eina_stringshare_del(global->interface);
             free(global);
          }
     }
   else
     goto event;

   if (!strcmp(interface, "wl_compositor"))
     {
        unsigned int request_version = 3;
#ifdef WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION
        request_version = 4;
#endif
        ewd->wl.compositor =
          wl_registry_bind(registry, id, &wl_compositor_interface,
                           MIN(version, request_version));
        ewd->wl.compositor_version = MIN(version, request_version);
     }
   else if (!strcmp(interface, "wl_subcompositor"))
     {
        ewd->wl.subcompositor =
          wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
     }
   else if (!strcmp(interface, "wl_shm"))
     {
        ewd->wl.shm =
          wl_registry_bind(registry, id, &wl_shm_interface, 1);
     }
   else if (!strcmp(interface, "wl_data_device_manager"))
     {
        ewd->wl.data_device_manager =
          wl_registry_bind(registry, id, &wl_data_device_manager_interface, 1);
     }
   else if (!strcmp(interface, "wl_shell"))
     {
        ewd->wl.wl_shell =
          wl_registry_bind(registry, id, &wl_shell_interface, 1);
     }
   else if ((!strcmp(interface, "xdg_shell")) &&
            (!getenv("EFL_WAYLAND_DONT_USE_XDG_SHELL")))
     {
        Ecore_Wl2_Window *window;

        ewd->wl.xdg_shell =
          wl_registry_bind(registry, id, &xdg_shell_interface, 1);
        xdg_shell_use_unstable_version(ewd->wl.xdg_shell, XDG_VERSION);
        xdg_shell_add_listener(ewd->wl.xdg_shell, &_xdg_shell_listener, NULL);

        EINA_INLIST_FOREACH(ewd->windows, window)
          _ecore_wl2_window_shell_surface_init(window);
     }
   else if (!strcmp(interface, "wl_output"))
     _ecore_wl2_output_add(ewd, id);
   else if (!strcmp(interface, "wl_seat"))
     _ecore_wl2_input_add(ewd, id, version);

event:
   /* allocate space for event structure */
   ev = calloc(1, sizeof(Ecore_Wl2_Event_Global));
   if (!ev) return;

   ev->id = id;
   ev->display = ewd;
   ev->version = version;
   ev->interface = eina_stringshare_add(interface);

   /* raise an event saying a new global has been added */
   ecore_event_add(ECORE_WL2_EVENT_GLOBAL_ADDED, ev,
                   _cb_global_event_free, NULL);
}

static void
_cb_global_remove(void *data, struct wl_registry *registry EINA_UNUSED, unsigned int id)
{
   Ecore_Wl2_Display *ewd;
   Ecore_Wl2_Global *global;
   Ecore_Wl2_Event_Global *ev;

   ewd = data;

   /* try to find this global in our hash */
   global = eina_hash_find(ewd->globals, &id);
   if (!global) return;

   /* allocate space for event structure */
   ev = calloc(1, sizeof(Ecore_Wl2_Event_Global));
   if (!ev) return;

   ev->id = id;
   ev->display = ewd;
   ev->version = global->version;
   ev->interface = eina_stringshare_add(global->interface);

   /* raise an event saying a global has been removed */
   ecore_event_add(ECORE_WL2_EVENT_GLOBAL_REMOVED, ev,
                   _cb_global_event_free, NULL);

   /* delete this global from our hash */
   if (ewd->globals) eina_hash_del_by_key(ewd->globals, &id);
}

static const struct wl_registry_listener _registry_listener =
{
   _cb_global_add,
   _cb_global_remove
};

static Eina_Bool
_cb_create_data(void *data, Ecore_Fd_Handler *hdl)
{
   Ecore_Wl2_Display *ewd;
   struct wl_event_loop *loop;

   ewd = data;

   if (_fatal_error) return ECORE_CALLBACK_CANCEL;

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_ERROR))
     {
        ERR("Received Fatal Error on Wayland Display");

        _fatal_error = EINA_TRUE;
        _ecore_wl2_display_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   loop = wl_display_get_event_loop(ewd->wl.display);
   wl_event_loop_dispatch(loop, 0);

   /* wl_display_flush_clients(ewd->wl.display); */

   return ECORE_CALLBACK_RENEW;
}

static void
_cb_create_prepare(void *data, Ecore_Fd_Handler *hdlr EINA_UNUSED)
{
   Ecore_Wl2_Display *ewd;

   ewd = data;
   wl_display_flush_clients(ewd->wl.display);
}

static Eina_Bool
_cb_connect_data(void *data, Ecore_Fd_Handler *hdl)
{
   Ecore_Wl2_Display *ewd;
   int ret = 0;

   ewd = data;

   if (_fatal_error) return ECORE_CALLBACK_CANCEL;

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_ERROR))
     {
        ERR("Received Fatal Error on Wayland Display");

        _fatal_error = EINA_TRUE;
        _ecore_wl2_display_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_READ))
     {
        ret = wl_display_dispatch(ewd->wl.display);
        if ((ret < 0) && ((errno != EAGAIN) && (errno != EINVAL)))
          {
             ERR("Received Fatal Error on Wayland Display");

             _fatal_error = EINA_TRUE;
             _ecore_wl2_display_signal_exit();

             return ECORE_CALLBACK_CANCEL;
          }
     }

   if (ecore_main_fd_handler_active_get(hdl, ECORE_FD_WRITE))
     {
        ret = wl_display_flush(ewd->wl.display);
        if (ret == 0)
          ecore_main_fd_handler_active_set(hdl, ECORE_FD_READ);

        if ((ret < 0) && ((errno != EAGAIN) && (errno != EINVAL)))
          {
             ERR("Received Fatal Error on Wayland Display");

             _fatal_error = EINA_TRUE;
             _ecore_wl2_display_signal_exit();

             return ECORE_CALLBACK_CANCEL;
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_cb_globals_hash_del(void *data)
{
   Ecore_Wl2_Global *global;

   global = data;

   eina_stringshare_del(global->interface);

   free(global);
}

static Eina_Bool
_cb_connect_idle(void *data)
{
   Ecore_Wl2_Display *ewd;
   int ret = 0;

   ewd = data;
   if (!ewd) return ECORE_CALLBACK_RENEW;

   if (_fatal_error) return ECORE_CALLBACK_CANCEL;

   ret = wl_display_get_error(ewd->wl.display);
   if (ret < 0) goto err;

   ret = wl_display_dispatch_pending(ewd->wl.display);
   if (ret < 0) goto err;

   ret = wl_display_flush(ewd->wl.display);
   if ((ret < 0) && (errno == EAGAIN))
     ecore_main_fd_handler_active_set(ewd->fd_hdl,
                                      (ECORE_FD_READ | ECORE_FD_WRITE));

   return ECORE_CALLBACK_RENEW;

err:
   if ((ret < 0) && ((errno != EAGAIN) && (errno != EINVAL)))
     {
        ERR("Wayland Socket Error: %s", strerror(errno));

        _fatal_error = EINA_TRUE;
        _ecore_wl2_display_signal_exit();

        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static void
_cb_sync_done(void *data, struct wl_callback *cb, uint32_t serial EINA_UNUSED)
{
   Ecore_Wl2_Event_Sync_Done *ev;
   Ecore_Wl2_Display *ewd;

   ewd = data;
   ewd->sync_done = EINA_TRUE;

   wl_callback_destroy(cb);

   ev = calloc(1, sizeof(Ecore_Wl2_Event_Sync_Done));
   if (!ev) return;

   ev->display = ewd;
   ecore_event_add(ECORE_WL2_EVENT_SYNC_DONE, ev, NULL, NULL);
}

static const struct wl_callback_listener _sync_listener =
{
   _cb_sync_done
};

static void
_ecore_wl2_display_cleanup(Ecore_Wl2_Display *ewd)
{
   Ecore_Wl2_Output *output;
   Ecore_Wl2_Input *input;
   Eina_Inlist *tmp;

   if (--ewd->refs) return;

   if (ewd->xkb_context) xkb_context_unref(ewd->xkb_context);

   /* free each input */
   EINA_INLIST_FOREACH_SAFE(ewd->inputs, tmp, input)
     _ecore_wl2_input_del(input);

   /* free each output */
   EINA_INLIST_FOREACH_SAFE(ewd->outputs, tmp, output)
     _ecore_wl2_output_del(output);

   if (ewd->idle_enterer) ecore_idle_enterer_del(ewd->idle_enterer);

   if (ewd->fd_hdl) ecore_main_fd_handler_del(ewd->fd_hdl);

   eina_hash_free(ewd->globals);

   if (ewd->wl.xdg_shell) xdg_shell_destroy(ewd->wl.xdg_shell);
   if (ewd->wl.wl_shell) wl_shell_destroy(ewd->wl.wl_shell);
   if (ewd->wl.shm) wl_shm_destroy(ewd->wl.shm);
   if (ewd->wl.data_device_manager)
     wl_data_device_manager_destroy(ewd->wl.data_device_manager);
   if (ewd->wl.compositor) wl_compositor_destroy(ewd->wl.compositor);
   if (ewd->wl.subcompositor) wl_subcompositor_destroy(ewd->wl.subcompositor);

   if (ewd->wl.registry) wl_registry_destroy(ewd->wl.registry);
}

Ecore_Wl2_Window *
_ecore_wl2_display_window_surface_find(Ecore_Wl2_Display *display, struct wl_surface *wl_surface)
{
   Ecore_Wl2_Window *window;

   if ((!display) || (!wl_surface)) return NULL;

   EINA_INLIST_FOREACH(display->windows, window)
     {
        if ((window->surface) &&
            (window->surface == wl_surface))
          return window;
     }

   return NULL;
}

EAPI Ecore_Wl2_Display *
ecore_wl2_display_create(const char *name)
{
   Ecore_Wl2_Display *ewd;
   struct wl_event_loop *loop;

   if (!_server_displays)
     _server_displays = eina_hash_string_superfast_new(NULL);

   if (name)
     {
        /* someone wants to create a server with a specific display */

        /* check hash of cached server displays for this name */
        ewd = eina_hash_find(_server_displays, name);
        if (ewd) goto found;
     }

   /* allocate space for display structure */
   ewd = calloc(1, sizeof(Ecore_Wl2_Display));
   if (!ewd) return NULL;

   ewd->refs++;
   ewd->pid = getpid();

   /* try to create new wayland display */
   ewd->wl.display = wl_display_create();
   if (!ewd->wl.display)
     {
        ERR("Could not create wayland display");
        goto create_err;
     }

   if (!name)
     {
        const char *n;

        n = wl_display_add_socket_auto(ewd->wl.display);
        if (!n)
          {
             ERR("Failed to add display socket");
             goto socket_err;
          }

        ewd->name = strdup(n);
     }
   else
     {
        if (wl_display_add_socket(ewd->wl.display, name))
          {
             ERR("Failed to add display socket");
             goto socket_err;
          }

        ewd->name = strdup(name);
     }

   setenv("WAYLAND_DISPLAY", ewd->name, 1);
   DBG("WAYLAND_DISPLAY: %s", ewd->name);

   loop = wl_display_get_event_loop(ewd->wl.display);

   ewd->fd_hdl =
     ecore_main_fd_handler_add(wl_event_loop_get_fd(loop),
                               ECORE_FD_READ | ECORE_FD_WRITE | ECORE_FD_ERROR,
                               _cb_create_data, ewd, NULL, NULL);

   ecore_main_fd_handler_prepare_callback_set(ewd->fd_hdl,
                                              _cb_create_prepare, ewd);

   /* add this new server display to hash */
   eina_hash_add(_server_displays, ewd->name, ewd);

   return ewd;

socket_err:
   wl_display_destroy(ewd->wl.display);

create_err:
   free(ewd);
   return NULL;

found:
   ewd->refs++;
   return ewd;
}

static Eina_Bool
_ecore_wl2_display_sync_get(void)
{
   Ecore_Wl2_Display *sewd;
   Eina_Iterator *itr;
   Eina_Bool ret = EINA_TRUE;
   void *data;

   if (eina_hash_population(_server_displays) < 1) return ret;

   itr = eina_hash_iterator_data_new(_server_displays);
   while (eina_iterator_next(itr, &data))
     {
        sewd = (Ecore_Wl2_Display *)data;
        if (sewd->pid == getpid())
          {
             ret = EINA_FALSE;
             break;
          }
     }
   eina_iterator_free(itr);

   return ret;
}

EAPI Ecore_Wl2_Display *
ecore_wl2_display_connect(const char *name)
{
   Ecore_Wl2_Display *ewd;
   Eina_Bool sync = EINA_TRUE;
   struct wl_callback *cb;
   const char *n;
   Eina_Bool hash_create = !_client_displays;

   if (!_client_displays)
     _client_displays = eina_hash_string_superfast_new(NULL);

   if (!name)
     {
        /* client wants to connect to default display */
        n = getenv("WAYLAND_DISPLAY");
        if (n)
          {
             /* we have a default wayland display */

             /* check hash of cached client displays for this name */
             ewd = eina_hash_find(_client_displays, n);
             if (ewd) goto found;
          }
     }
   else
     {
        /* client wants to connect to specific display */

        /* check hash of cached client displays for this name */
        ewd = eina_hash_find(_client_displays, name);
        if (ewd) goto found;
     }

   if ((!name) && (!n))
     {
        ERR("No Wayland Display Running");
        goto name_err;
     }

   /* allocate space for display structure */
   ewd = calloc(1, sizeof(Ecore_Wl2_Display));
   if (!ewd) return NULL;

   ewd->refs++;

   if (name)
     ewd->name = strdup(name);
   else if (n)
     ewd->name = strdup(n);

   ewd->globals = eina_hash_int32_new(_cb_globals_hash_del);

   /* try to connect to wayland display with this name */
   ewd->wl.display = wl_display_connect(ewd->name);
   if (!ewd->wl.display)
     {
        ERR("Could not connect to display %s", ewd->name);
        goto connect_err;
     }

   ewd->fd_hdl =
     ecore_main_fd_handler_add(wl_display_get_fd(ewd->wl.display),
                               ECORE_FD_READ | ECORE_FD_ERROR,
                               _cb_connect_data, ewd, NULL, NULL);

   ewd->idle_enterer = ecore_idle_enterer_add(_cb_connect_idle, ewd);

   ewd->wl.registry = wl_display_get_registry(ewd->wl.display);
   wl_registry_add_listener(ewd->wl.registry, &_registry_listener, ewd);

   ewd->xkb_context = xkb_context_new(0);
   if (!ewd->xkb_context) goto context_err;

   /* add this new client display to hash */
   eina_hash_add(_client_displays, ewd->name, ewd);

   /* check server display hash and match on pid. If match, skip sync */
   sync = _ecore_wl2_display_sync_get();

   cb = wl_display_sync(ewd->wl.display);
   wl_callback_add_listener(cb, &_sync_listener, ewd);

   if (sync)
     {
        /* NB: If we are connecting (as a client), then we will need to setup
         * a callback for display_sync and wait for it to complete. There is no
         * other option here as we need the compositor, shell, etc, to be setup
         * before we can allow a user to make use of the API functions */
        while (!ewd->sync_done)
          wl_display_dispatch(ewd->wl.display);
     }

   return ewd;

context_err:
   ecore_main_fd_handler_del(ewd->fd_hdl);
   wl_registry_destroy(ewd->wl.registry);
   wl_display_disconnect(ewd->wl.display);

connect_err:
   eina_hash_free(ewd->globals);
   free(ewd->name);
   free(ewd);
   return NULL;

name_err:
   if (hash_create)
     {
        eina_hash_free(_client_displays);
        _client_displays = NULL;
     }
   return NULL;

found:
   ewd->refs++;
   return ewd;
}

EAPI void
ecore_wl2_display_disconnect(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);
   _ecore_wl2_display_cleanup(display);
   if (display->refs <= 0)
     {
        wl_display_disconnect(display->wl.display);

        /* remove this client display from hash */
        eina_hash_del_by_key(_client_displays, display->name);

        free(display->name);
        free(display);
     }
}

EAPI void
ecore_wl2_display_destroy(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);
   _ecore_wl2_display_cleanup(display);
   if (display->refs <= 0)
     {
        wl_display_destroy(display->wl.display);

        /* remove this client display from hash */
        eina_hash_del_by_key(_server_displays, display->name);

        free(display->name);
        free(display);
     }
}

EAPI void
ecore_wl2_display_terminate(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN(display);
   wl_display_terminate(display->wl.display);
}

EAPI struct wl_display *
ecore_wl2_display_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.display;
}

EAPI struct wl_shm *
ecore_wl2_display_shm_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->wl.shm;
}

EAPI Eina_Iterator *
ecore_wl2_display_globals_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);
   return display->globals ? eina_hash_iterator_data_new(display->globals) : NULL;
}

EAPI void
ecore_wl2_display_screen_size_get(Ecore_Wl2_Display *display, int *w, int *h)
{
   Ecore_Wl2_Output *output;
   int ow = 0, oh = 0;

   EINA_SAFETY_ON_NULL_RETURN(display);

   if (w) *w = 0;
   if (h) *h = 0;

   EINA_INLIST_FOREACH(display->outputs, output)
     {
        switch (output->transform)
          {
           case WL_OUTPUT_TRANSFORM_90:
           case WL_OUTPUT_TRANSFORM_270:
           case WL_OUTPUT_TRANSFORM_FLIPPED_90:
           case WL_OUTPUT_TRANSFORM_FLIPPED_270:
             ow += output->geometry.h;
             oh += output->geometry.w;
             break;
           default:
             ow += output->geometry.w;
             oh += output->geometry.h;
             break;
          }
     }

   if (w) *w = ow;
   if (h) *h = oh;
}

EAPI Ecore_Wl2_Window *
ecore_wl2_display_window_find(Ecore_Wl2_Display *display, unsigned int id)
{
   Ecore_Wl2_Window *window;

   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);

   EINA_INLIST_FOREACH(display->windows, window)
     if (window->id == (int)id) return window;

   return NULL;
}

EAPI struct wl_registry *
ecore_wl2_display_registry_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, NULL);

   return display->wl.registry;
}

EAPI int
ecore_wl2_display_compositor_version_get(Ecore_Wl2_Display *display)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(display, 0);

   return display->wl.compositor_version;
}
