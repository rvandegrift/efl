#ifndef _ECORE_EVAS_WAYLAND_PRIVATE_H_
# define _ECORE_EVAS_WAYLAND_PRIVATE_H_

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# define ECORE_EVAS_INTERNAL

# ifndef ELEMENTARY_H
//#define LOGFNS 1
#  ifdef LOGFNS
#   include <stdio.h>
#   define LOGFN(fl, ln, fn) \
   printf("-ECORE_EVAS-WL: %25s: %5i - %s\n", fl, ln, fn);
#  else
#   define LOGFN(fl, ln, fn)
#  endif

#  include <Eina.h>
#  include <Ecore.h>
#  include <Ecore_Input.h>
#  include <Ecore_Input_Evas.h>
#  include <Ecore_Wl2.h>

#  include <Ecore_Evas.h>
# endif

# include "ecore_wl2_private.h"
# include "ecore_private.h"
# include "ecore_evas_private.h"
# include "ecore_evas_wayland.h"

typedef struct _Ecore_Evas_Engine_Wl_Data Ecore_Evas_Engine_Wl_Data;

struct _Ecore_Evas_Engine_Wl_Data 
{
   Ecore_Wl2_Display *display;
   Eina_List *regen_objs;
   Ecore_Wl2_Window *parent, *win;
   Ecore_Event_Handler *sync_handler;
   int fx, fy, fw, fh;
   Eina_Rectangle content;
   struct wl_callback *anim_callback;
   int x_rel;
   int y_rel;
   uint32_t timestamp;
   Eina_List *devices_list;
   Eina_Bool dragging : 1;
   Eina_Bool sync_done : 1;
   Eina_Bool defer_show : 1;
   Eina_Bool reset_pending : 1;
};

Ecore_Evas_Interface_Wayland *_ecore_evas_wl_interface_new(void);

int  _ecore_evas_wl_common_init(void);
int  _ecore_evas_wl_common_shutdown(void);
void _ecore_evas_wl_common_free(Ecore_Evas *ee);
void _ecore_evas_wl_common_callback_resize_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_move_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_delete_request_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_focus_in_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_focus_out_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_mouse_in_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_callback_mouse_out_set(Ecore_Evas *ee, void (*func)(Ecore_Evas *ee));
void _ecore_evas_wl_common_move(Ecore_Evas *ee, int x, int y);
void _ecore_evas_wl_common_resize(Ecore_Evas *ee, int w, int h);
void _ecore_evas_wl_common_move_resize(Ecore_Evas *ee, int x, int y, int w, int h);
void _ecore_evas_wl_common_raise(Ecore_Evas *ee);
void _ecore_evas_wl_common_title_set(Ecore_Evas *ee, const char *title);
void _ecore_evas_wl_common_name_class_set(Ecore_Evas *ee, const char *n, const char *c);
void _ecore_evas_wl_common_size_min_set(Ecore_Evas *ee, int w, int h);
void _ecore_evas_wl_common_size_max_set(Ecore_Evas *ee, int w, int h);
void _ecore_evas_wl_common_size_base_set(Ecore_Evas *ee, int w, int h);
void _ecore_evas_wl_common_size_step_set(Ecore_Evas *ee, int w, int h);
void _ecore_evas_wl_common_aspect_set(Ecore_Evas *ee, double aspect);
void _ecore_evas_wl_common_object_cursor_set(Ecore_Evas *ee, Evas_Object *obj, int layer, int hot_x, int hot_y);
void _ecore_evas_wl_common_object_cursor_unset(Ecore_Evas *ee);
void _ecore_evas_wl_common_layer_set(Ecore_Evas *ee, int layer);
void _ecore_evas_wl_common_iconified_set(Ecore_Evas *ee, Eina_Bool on);
void _ecore_evas_wl_common_maximized_set(Ecore_Evas *ee, Eina_Bool on);
void _ecore_evas_wl_common_fullscreen_set(Ecore_Evas *ee, Eina_Bool on);
void _ecore_evas_wl_common_ignore_events_set(Ecore_Evas *ee, int ignore);
int  _ecore_evas_wl_common_pre_render(Ecore_Evas *ee);
/* int  _ecore_evas_wl_common_render_updates(Ecore_Evas *ee); */
void _ecore_evas_wl_common_post_render(Ecore_Evas *ee);
int  _ecore_evas_wl_common_render(Ecore_Evas *ee);
void _ecore_evas_wl_common_render_flush_pre(void *data, Evas *evas EINA_UNUSED, void *event EINA_UNUSED);
void _ecore_evas_wl_common_screen_geometry_get(const Ecore_Evas *ee, int *x, int *y, int *w, int *h);
void _ecore_evas_wl_common_screen_dpi_get(const Ecore_Evas *ee, int *xdpi, int *ydpi);
void _ecore_evas_wl_common_render_pre(void *data, Evas *evas EINA_UNUSED, void *event);
void _ecore_evas_wl_common_render_updates(void *data, Evas *evas, void *event);
void _ecore_evas_wl_common_rotation_set(Ecore_Evas *ee, int rotation, int resize);
void _ecore_evas_wl_common_borderless_set(Ecore_Evas *ee, Eina_Bool on);
void _ecore_evas_wl_common_withdrawn_set(Ecore_Evas *ee, Eina_Bool on);
void _ecore_evas_wl_common_show(Ecore_Evas *ee);
void _ecore_evas_wl_common_hide(Ecore_Evas *ee);
void _ecore_evas_wl_common_alpha_set(Ecore_Evas *ee, int alpha);
void _ecore_evas_wl_common_transparent_set(Ecore_Evas *ee, int transparent);

void _ecore_evas_wl_common_frame_callback_clean(Ecore_Evas *ee);
void _ecore_evas_wl_common_pointer_xy_get(const Ecore_Evas *ee, Evas_Coord *x, Evas_Coord *y);

void _ecore_evas_wl_common_pointer_device_xy_get(const Ecore_Evas *ee, const Efl_Input_Device *pointer, Evas_Coord *x, Evas_Coord *y);

Ecore_Evas *_ecore_evas_wl_common_new_internal(const char *disp_name, unsigned int parent, int x, int y, int w, int h, Eina_Bool frame, const char *engine_name);

extern Eina_List *ee_list;

#endif /* _ECORE_EVAS_WAYLAND_PRIVATE_H_ */
