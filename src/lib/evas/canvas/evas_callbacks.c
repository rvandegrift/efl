#define EFL_CANVAS_OBJECT_BETA
#define EVAS_CANVAS_BETA

#include "evas_common_private.h"
#include "evas_private.h"

#define EFL_INTERNAL_UNSTABLE
#include "interfaces/efl_common_internal.h"

int _evas_event_counter = 0;

EVAS_MEMPOOL(_mp_pc);

extern Eina_Hash* signals_hash_table;

/**
 * Evas events descriptions for Eo.
 */
#define DEFINE_EVAS_CALLBACKS(LAST, ...)                                 \
  static const Eo_Event_Description *_legacy_evas_callback_table(unsigned int index) \
  {                                                                     \
     static const Eo_Event_Description *internals[LAST] = { NULL };       \
                                                                        \
     if (internals[0] == NULL)                                          \
       {                                                                \
          memcpy(internals,                                             \
                 ((const Eo_Event_Description*[]) { __VA_ARGS__ }),            \
                 sizeof ((const Eo_Event_Description *[]) { __VA_ARGS__ }));    \
       }                                                                \
     return internals[index];                                           \
  }

DEFINE_EVAS_CALLBACKS(EVAS_CALLBACK_LAST,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_IN,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_OUT,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_DOWN,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_UP,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_MOVE,
                      EFL_CANVAS_OBJECT_EVENT_MOUSE_WHEEL,
                      EFL_CANVAS_OBJECT_EVENT_MULTI_DOWN,
                      EFL_CANVAS_OBJECT_EVENT_MULTI_UP,
                      EFL_CANVAS_OBJECT_EVENT_MULTI_MOVE,
                      EFL_CANVAS_OBJECT_EVENT_FREE,
                      EFL_CANVAS_OBJECT_EVENT_KEY_DOWN,
                      EFL_CANVAS_OBJECT_EVENT_KEY_UP,
                      EFL_CANVAS_OBJECT_EVENT_FOCUS_IN,
                      EFL_CANVAS_OBJECT_EVENT_FOCUS_OUT,
                      EFL_GFX_EVENT_SHOW,
                      EFL_GFX_EVENT_HIDE,
                      EFL_GFX_EVENT_MOVE,
                      EFL_GFX_EVENT_RESIZE,
                      EFL_GFX_EVENT_RESTACK,
                      EFL_CANVAS_OBJECT_EVENT_DEL,
                      EFL_CANVAS_OBJECT_EVENT_HOLD,
                      EFL_GFX_EVENT_CHANGE_SIZE_HINTS,
                      EFL_IMAGE_EVENT_PRELOAD,
                      EFL_CANVAS_EVENT_FOCUS_IN,
                      EFL_CANVAS_EVENT_FOCUS_OUT,
                      EVAS_CANVAS_EVENT_RENDER_FLUSH_PRE,
                      EVAS_CANVAS_EVENT_RENDER_FLUSH_POST,
                      EFL_CANVAS_EVENT_OBJECT_FOCUS_IN,
                      EFL_CANVAS_EVENT_OBJECT_FOCUS_OUT,
                      EFL_IMAGE_EVENT_UNLOAD,
                      EFL_CANVAS_EVENT_RENDER_PRE,
                      EFL_CANVAS_EVENT_RENDER_POST,
                      EFL_IMAGE_EVENT_RESIZE,
                      EFL_CANVAS_EVENT_DEVICE_CHANGED,
                      EVAS_CANVAS_EVENT_AXIS_UPDATE,
                      EVAS_CANVAS_EVENT_VIEWPORT_RESIZE );

typedef struct
{
   EINA_INLIST;
   Evas_Object_Event_Cb func;
   void *data;
   Evas_Callback_Type type;
} _eo_evas_object_cb_info;

typedef struct
{
   EINA_INLIST;
   Evas_Event_Cb func;
   void *data;
   Evas_Callback_Type type;
} _eo_evas_cb_info;

static void
_eo_evas_object_cb(void *data, const Eo_Event *event)
{
   _eo_evas_object_cb_info *info = data;
   Evas *evas = evas_object_evas_get(event->object);
   if (info->func) info->func(info->data, evas, event->object, event->info);
}

static void
_eo_evas_cb(void *data, const Eo_Event *event)
{
   _eo_evas_cb_info *info = data;
   if (info->func) info->func(info->data, event->object, event->info);
}

void
_evas_post_event_callback_call(Evas *eo_e, Evas_Public_Data *e)
{
   Evas_Post_Callback *pc;
   int skip = 0;
   static int first_run = 1; // FIXME: This is a workaround to prevent this
                             // function from being called recursively.

   if (e->delete_me || (!first_run)) return;
   _evas_walk(e);
   first_run = 0;
   EINA_LIST_FREE(e->post_events, pc)
     {
        if ((!skip) && (!e->delete_me) && (!pc->delete_me))
          {
             if (!pc->func((void*)pc->data, eo_e)) skip = 1;
          }
        EVAS_MEMPOOL_FREE(_mp_pc, pc);
     }
   first_run = 1;
   _evas_unwalk(e);
}

void
_evas_post_event_callback_free(Evas *eo_e)
{
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   Evas_Post_Callback *pc;

   EINA_LIST_FREE(e->post_events, pc)
     {
        EVAS_MEMPOOL_FREE(_mp_pc, pc);
     }
}

void
evas_object_event_callback_all_del(Evas_Object *eo_obj)
{
   _eo_evas_object_cb_info *info;
   Eina_Inlist *itr;
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return;
   if (!obj->callbacks) return;
   EINA_INLIST_FOREACH_SAFE(obj->callbacks, itr, info)
     {
        eo_event_callback_del(eo_obj, _legacy_evas_callback_table(info->type), _eo_evas_object_cb, info);

        obj->callbacks =
           eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
        free(info);
     }
}

void
evas_object_event_callback_cleanup(Evas_Object *eo_obj)
{
   evas_object_event_callback_all_del(eo_obj);
}

void
evas_event_callback_all_del(Evas *eo_e)
{
   _eo_evas_object_cb_info *info;
   Eina_Inlist *itr;
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (!e) return;
   if (!e->callbacks) return;

   EINA_INLIST_FOREACH_SAFE(e->callbacks, itr, info)
     {
        eo_event_callback_del(eo_e, _legacy_evas_callback_table(info->type), _eo_evas_cb, info);

        e->callbacks =
           eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
        free(info);
     }
}

void
evas_event_callback_cleanup(Evas *eo_e)
{
   evas_event_callback_all_del(eo_e);
}

void
evas_event_callback_call(Evas *eo_e, Evas_Callback_Type type, void *event_info)
{
   eo_event_callback_call(eo_e, _legacy_evas_callback_table(type), event_info);
}

void
evas_object_event_callback_call(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj,
                                Evas_Callback_Type type, void *event_info, int event_id,
                                const Eo_Event_Description *eo_event_desc, Efl_Event *eo_event_info)
{
   /* MEM OK */
   Evas_Button_Flags flags = EVAS_BUTTON_NONE;
   Evas_Public_Data *e;

   if (!obj) return;
   if ((obj->delete_me) || (!obj->layer)) return;
   if ((obj->last_event == event_id) &&
       (obj->last_event_type == type)) return;
   if (obj->last_event > event_id)
     {
        if ((obj->last_event_type == EVAS_CALLBACK_MOUSE_OUT) &&
            ((type >= EVAS_CALLBACK_MOUSE_DOWN) &&
             (type <= EVAS_CALLBACK_MULTI_MOVE)))
          {
             return;
          }
     }
   obj->last_event = event_id;
   obj->last_event_type = type;
   if (!(e = obj->layer->evas)) return;

   _evas_walk(e);

   if ((type == EVAS_CALLBACK_MOVE) && (obj->move_ref == 0))
     goto nothing_here;

   switch (type)
     {
      case EVAS_CALLBACK_MOUSE_DOWN:
           {
              Evas_Event_Mouse_Down *ev = event_info;

              flags = ev->flags;
              if (ev->flags & (EVAS_BUTTON_DOUBLE_CLICK | EVAS_BUTTON_TRIPLE_CLICK))
                {
                   if (obj->last_mouse_down_counter < (e->last_mouse_down_counter - 1))
                     ev->flags &= ~(EVAS_BUTTON_DOUBLE_CLICK | EVAS_BUTTON_TRIPLE_CLICK);
                }
              obj->last_mouse_down_counter = e->last_mouse_down_counter;
              if (eo_event_info)
                {
                   efl_event_pointer_button_flags_set(eo_event_info, ev->flags);
                }
              break;
           }
      case EVAS_CALLBACK_MOUSE_UP:
           {
              Evas_Event_Mouse_Up *ev = event_info;

              flags = ev->flags;
              if (ev->flags & (EVAS_BUTTON_DOUBLE_CLICK | EVAS_BUTTON_TRIPLE_CLICK))
                {
                   if (obj->last_mouse_up_counter < (e->last_mouse_up_counter - 1))
                     ev->flags &= ~(EVAS_BUTTON_DOUBLE_CLICK | EVAS_BUTTON_TRIPLE_CLICK);
                }
              obj->last_mouse_up_counter = e->last_mouse_up_counter;
              if (eo_event_info)
                {
                   efl_event_pointer_button_flags_set(eo_event_info, ev->flags);
                }
              break;
           }
      default:
         break;
     }

   /* legacy callbacks - relying on Efl.Canvas.Object events */
   eo_event_callback_call(eo_obj, _legacy_evas_callback_table(type), event_info);

   /* new input events */
   if (eo_event_desc)
     {
        Efl_Event_Flags *pevflags = NULL;

#define EV_CASE(TYPE, NEWTYPE, Type) \
   case EVAS_CALLBACK_ ## TYPE: \
   pevflags = &(((Evas_Event_ ## Type *) event_info)->event_flags); \
   break
        switch (type)
          {
           EV_CASE(MOUSE_MOVE,  POINTER_MOVE,  Mouse_Move);
           EV_CASE(MOUSE_OUT,   POINTER_OUT,   Mouse_Out);
           EV_CASE(MOUSE_IN,    POINTER_IN,    Mouse_In);
           EV_CASE(MOUSE_DOWN , POINTER_DOWN,  Mouse_Down);
           EV_CASE(MOUSE_UP,    POINTER_UP,    Mouse_Up);
           EV_CASE(MULTI_MOVE,  POINTER_MOVE,  Multi_Move);
           EV_CASE(MULTI_DOWN,  POINTER_DOWN,  Multi_Down);
           EV_CASE(MULTI_UP,    POINTER_UP,    Multi_Up);
           EV_CASE(MOUSE_WHEEL, POINTER_WHEEL, Mouse_Wheel);
           EV_CASE(KEY_DOWN,    KEY_DOWN,      Key_Down);
           EV_CASE(KEY_UP,      KEY_UP,        Key_Up);
           default: break;
          }
#undef EV_CASE

        if (pevflags) efl_event_flags_set(eo_event_info, *pevflags);
        eo_event_callback_call(eo_obj, eo_event_desc, eo_event_info);
     }

   if (type == EVAS_CALLBACK_MOUSE_DOWN)
     {
        Evas_Event_Mouse_Down *ev = event_info;
        ev->flags = flags;
        if (eo_event_info)
          efl_event_pointer_button_flags_set(eo_event_info, ev->flags);
     }
   else if (type == EVAS_CALLBACK_MOUSE_UP)
     {
        Evas_Event_Mouse_Up *ev = event_info;
        ev->flags = flags;
        if (eo_event_info)
          efl_event_pointer_button_flags_set(eo_event_info, ev->flags);
     }

 nothing_here:
   if (!obj->no_propagate)
     {
        if ((obj->smart.parent) && (type != EVAS_CALLBACK_FREE) &&
              (type <= EVAS_CALLBACK_KEY_UP))
          {
             Evas_Object_Protected_Data *smart_parent = eo_data_scope_get(obj->smart.parent, EFL_CANVAS_OBJECT_CLASS);
             evas_object_event_callback_call(obj->smart.parent, smart_parent, type, event_info, event_id, eo_event_desc, eo_event_info);
          }
     }
   _evas_unwalk(e);
}

EAPI void
evas_object_event_callback_add(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func, const void *data)
{
   evas_object_event_callback_priority_add(eo_obj, type,
                                           EVAS_CALLBACK_PRIORITY_DEFAULT, func, data);
}

EAPI void
evas_object_event_callback_priority_add(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Callback_Priority priority, Evas_Object_Event_Cb func, const void *data)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return;
   MAGIC_CHECK_END();

   if (!obj) return;
   if (!func) return;

   _eo_evas_object_cb_info *cb_info = calloc(1, sizeof(*cb_info));
   cb_info->func = func;
   cb_info->data = (void *)data;
   cb_info->type = type;

   const Eo_Event_Description *desc = _legacy_evas_callback_table(type);
   eo_event_callback_priority_add(eo_obj, desc, priority, _eo_evas_object_cb, cb_info);

   obj->callbacks =
      eina_inlist_append(obj->callbacks, EINA_INLIST_GET(cb_info));
}

EAPI void *
evas_object_event_callback_del(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func)
{
   _eo_evas_object_cb_info *info;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return NULL;
   if (!func) return NULL;

   if (!obj->callbacks) return NULL;

   EINA_INLIST_REVERSE_FOREACH(obj->callbacks, info)
     {
        if ((info->func == func) && (info->type == type))
          {
             void *tmp = info->data;
             eo_event_callback_del(eo_obj, _legacy_evas_callback_table(type), _eo_evas_object_cb, info);

             obj->callbacks =
                eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

EAPI void *
evas_object_event_callback_del_full(Evas_Object *eo_obj, Evas_Callback_Type type, Evas_Object_Event_Cb func, const void *data)
{
   _eo_evas_object_cb_info *info;

   MAGIC_CHECK(eo_obj, Evas_Object, MAGIC_OBJ);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (!obj) return NULL;
   if (!func) return NULL;

   if (!obj->callbacks) return NULL;

   EINA_INLIST_FOREACH(obj->callbacks, info)
     {
        if ((info->func == func) && (info->type == type) && info->data == data)
          {
             void *tmp = info->data;
             eo_event_callback_del(eo_obj, _legacy_evas_callback_table(type), _eo_evas_object_cb, info);

             obj->callbacks =
                eina_inlist_remove(obj->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

EAPI void
evas_event_callback_add(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func, const void *data)
{
   evas_event_callback_priority_add(eo_e, type, EVAS_CALLBACK_PRIORITY_DEFAULT,
                                    func, data);
}

EAPI void
evas_event_callback_priority_add(Evas *eo_e, Evas_Callback_Type type, Evas_Callback_Priority priority, Evas_Event_Cb func, const void *data)
{
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   if (!func) return;

   _eo_evas_cb_info *cb_info = calloc(1, sizeof(*cb_info));
   cb_info->func = func;
   cb_info->data = (void *)data;
   cb_info->type = type;

   const Eo_Event_Description *desc = _legacy_evas_callback_table(type);
   eo_event_callback_priority_add(eo_e, desc, priority, _eo_evas_cb, cb_info);

   e->callbacks = eina_inlist_append(e->callbacks, EINA_INLIST_GET(cb_info));
}

EAPI void *
evas_event_callback_del(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func)
{
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   _eo_evas_cb_info *info;

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();

   if (!e) return NULL;
   if (!func) return NULL;

   if (!e->callbacks) return NULL;

   EINA_INLIST_REVERSE_FOREACH(e->callbacks, info)
     {
        if ((info->func == func) && (info->type == type))
          {
             void *tmp = info->data;
             eo_event_callback_del(eo_e, _legacy_evas_callback_table(type), _eo_evas_cb, info);

             e->callbacks =
                eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

EAPI void *
evas_event_callback_del_full(Evas *eo_e, Evas_Callback_Type type, Evas_Event_Cb func, const void *data)
{
   _eo_evas_cb_info *info;

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return NULL;
   MAGIC_CHECK_END();
   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);

   if (!e) return NULL;
   if (!func) return NULL;

   if (!e->callbacks) return NULL;

   EINA_INLIST_FOREACH(e->callbacks, info)
     {
        if ((info->func == func) && (info->type == type) && (info->data == data))
          {
             void *tmp = info->data;
             eo_event_callback_del(eo_e, _legacy_evas_callback_table(type), _eo_evas_cb, info);

             e->callbacks =
                eina_inlist_remove(e->callbacks, EINA_INLIST_GET(info));
             free(info);
             return tmp;
          }
     }
   return NULL;
}

EAPI void
evas_post_event_callback_push(Evas *eo_e, Evas_Object_Event_Post_Cb func, const void *data)
{
   Evas_Post_Callback *pc;

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   if (!e) return;
   EVAS_MEMPOOL_INIT(_mp_pc, "evas_post_callback", Evas_Post_Callback, 64, );
   pc = EVAS_MEMPOOL_ALLOC(_mp_pc, Evas_Post_Callback);
   if (!pc) return;
   EVAS_MEMPOOL_PREP(_mp_pc, pc, Evas_Post_Callback);
   if (e->delete_me) return;

   pc->func = func;
   pc->data = data;
   e->post_events = eina_list_prepend(e->post_events, pc);
}

EAPI void
evas_post_event_callback_remove(Evas *eo_e, Evas_Object_Event_Post_Cb func)
{
   Evas_Post_Callback *pc;
   Eina_List *l;

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   if (!e) return;
   EINA_LIST_FOREACH(e->post_events, l, pc)
     {
        if (pc->func == func)
          {
             pc->delete_me = 1;
             return;
          }
     }
}

EAPI void
evas_post_event_callback_remove_full(Evas *eo_e, Evas_Object_Event_Post_Cb func, const void *data)
{
   Evas_Post_Callback *pc;
   Eina_List *l;

   MAGIC_CHECK(eo_e, Evas, MAGIC_EVAS);
   return;
   MAGIC_CHECK_END();

   Evas_Public_Data *e = eo_data_scope_get(eo_e, EVAS_CANVAS_CLASS);
   if (!e) return;
   EINA_LIST_FOREACH(e->post_events, l, pc)
     {
        if ((pc->func == func) && (pc->data == data))
          {
             pc->delete_me = 1;
             return;
          }
     }
}
