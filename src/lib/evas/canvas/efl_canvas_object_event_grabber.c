#include "evas_common_private.h"
#include "evas_private.h"
#include "efl_canvas_object_event_grabber.eo.h"

#define MY_CLASS EFL_CANVAS_OBJECT_EVENT_GRABBER_CLASS
#define MY_CLASS_NAME "Efl_Object_Event_Grabber"
#define MY_CLASS_NAME_LEGACY "evas_object_event_grabber"


struct _Efl_Object_Event_Grabber_Data
{
   Eo *rect;
   Eina_List *contained;

   Eina_Bool vis : 1;
   Eina_Bool freeze : 1;
   Eina_Bool restack : 1;
};

typedef struct Efl_Object_Event_Grabber_Iterator
{
   Eina_Iterator iterator;

   Eina_Iterator *itl;
   Eo *parent;
} Efl_Object_Event_Grabber_Iterator;


static Eina_Bool
_efl_canvas_group_group_iterator_next(Efl_Object_Event_Grabber_Iterator *it, void **data)
{
   return eina_iterator_next(it->itl, data);
}

static Evas_Object *
_efl_canvas_group_group_iterator_get_container(Efl_Object_Event_Grabber_Iterator *it)
{
   return it->parent;
}

static void
_efl_canvas_group_group_iterator_free(Efl_Object_Event_Grabber_Iterator *it)
{
   efl_unref(it->parent);
   free(it);
}

EOLIAN static Eina_Iterator*
_efl_canvas_object_event_grabber_efl_canvas_group_group_children_iterate(const Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Efl_Object_Event_Grabber_Iterator *it;

   if (!pd->contained) return NULL;

   it = calloc(1, sizeof(Efl_Object_Event_Grabber_Iterator));
   if (!it) return NULL;

   EINA_MAGIC_SET(&it->iterator, EINA_MAGIC_ITERATOR);
   it->parent = efl_ref(eo_obj);
   it->itl = eina_list_iterator_new(pd->contained);

   it->iterator.next = FUNC_ITERATOR_NEXT(_efl_canvas_group_group_iterator_next);
   it->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_efl_canvas_group_group_iterator_get_container);
   it->iterator.free = FUNC_ITERATOR_FREE(_efl_canvas_group_group_iterator_free);

   return &it->iterator;
}

static void
_stacking_verify(Efl_Object_Event_Grabber_Data *pd, Evas_Object_Protected_Data *obj)
{
   Eo *grabber;
   Evas_Object_Protected_Data *gobj, *i;

   grabber = efl_parent_get(pd->rect);
   gobj = efl_data_scope_get(grabber, EFL_CANVAS_OBJECT_CLASS);
   if (obj->layer->layer > gobj->layer->layer)
     {
        CRI("Cannot stack child object above event grabber object!");
        return;
     }
   if (obj->layer != gobj->layer) return;

   EINA_INLIST_REVERSE_FOREACH(EINA_INLIST_GET(gobj->layer->objects), i)
     {
        if (i == gobj) break;
        if (i == obj)
          {
             CRI("Cannot stack child object above event grabber object!");
             return;
          }
     }
}

static void
_child_insert(Efl_Object_Event_Grabber_Data *pd, Evas_Object_Protected_Data *obj)
{
   Evas_Object_Protected_Data *a, *i;
   Eina_List *l;
   Eina_Bool found = EINA_FALSE;

   if (pd->vis) _stacking_verify(pd, obj);

   EINA_LIST_REVERSE_FOREACH(pd->contained, l, a)
     {
        if (a->object == pd->rect)
          {
             found = EINA_TRUE;
             break;
          }
        if (a->layer->layer > obj->layer->layer) continue;
        if (a->layer->layer < obj->layer->layer)
          {
             found = EINA_TRUE;
             break;
          }
        EINA_INLIST_FOREACH(EINA_INLIST_GET(a->layer->objects), i)
          {
             if (a == i || obj == i)
               {
                  found = EINA_TRUE;
                  break;
               }
          }
        if (found) break;
     }

   if (found)
     pd->contained = eina_list_append_relative(pd->contained, obj, a);
   else
     pd->contained = eina_list_prepend(pd->contained, obj);
}

static void
_full_restack(Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;
   Evas_Object_Protected_Data *root = NULL;
   Eina_List *list = NULL;
   EINA_LIST_FREE(pd->contained, obj)
     {
        if (obj->object == pd->rect)
          {
             root = obj;
             continue;
          }
        list = eina_list_append(list, obj);
     }

   pd->contained = eina_list_append(pd->contained, root);
   EINA_LIST_FREE(list, obj)
     _child_insert(pd, obj);
   pd->restack = 0;
}

static void
_efl_canvas_object_event_grabber_child_restack(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;
   Evas_Object_Protected_Data *obj = efl_data_scope_get(event->object, EFL_CANVAS_OBJECT_CLASS);

   if (pd->vis && pd->freeze)
     {
        pd->restack = 1;
        return;
     }
   pd->contained = eina_list_remove(pd->contained, obj);
   _child_insert(pd, obj);
}

static void
_efl_canvas_object_event_grabber_child_del(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;

   efl_canvas_group_member_del(efl_parent_get(pd->rect), event->object);
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_canvas_group_group_member_add(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, Eo *member)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(member, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *smart = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   EINA_SAFETY_ON_NULL_RETURN(obj);
   EINA_SAFETY_ON_NULL_RETURN(smart);

   if (member != pd->rect)
     {
        if (obj->delete_me)
          {
             CRI("Adding deleted object %p to smart obj %p", member, eo_obj);
             return;
          }
        if (smart->delete_me)
          {
             CRI("Adding object %p to deleted smart obj %p", member, eo_obj);
             return;
          }
        if (!smart->layer)
          {
             CRI("No evas surface associated with smart object (%p)", eo_obj);
             return;
          }
        if (!obj->layer)
          {
             CRI("No evas surface associated with member object (%p)", member);
             return;
          }
        if ((obj->layer && smart->layer) &&
            (obj->layer->evas != smart->layer->evas))
          {
             CRI("Adding object %p from Evas (%p) from another Evas (%p)", member, obj->layer->evas, smart->layer->evas);
             return;
          }
     }
   if (obj->events->parent == eo_obj) return;

   if (obj->smart.parent || obj->events->parent) evas_object_smart_member_del(member);
   EINA_COW_WRITE_BEGIN(evas_object_events_cow, obj->events, Evas_Object_Events_Data, events)
     events->parent = eo_obj;
   EINA_COW_WRITE_END(evas_object_events_cow, obj->events, events);
   _child_insert(pd, obj);
   efl_event_callback_add(member, EFL_EVENT_DEL, _efl_canvas_object_event_grabber_child_del, pd);
   if (member != pd->rect)
     efl_event_callback_add(member, EFL_GFX_EVENT_RESTACK, _efl_canvas_object_event_grabber_child_restack, pd);
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_canvas_group_group_member_del(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eo *member)
{
   Evas_Object_Protected_Data *obj = efl_data_scope_get(member, EFL_CANVAS_OBJECT_CLASS);

   efl_event_callback_del(member, EFL_EVENT_DEL, _efl_canvas_object_event_grabber_child_del, pd);
   efl_event_callback_del(member, EFL_GFX_EVENT_RESTACK, _efl_canvas_object_event_grabber_child_restack, pd);
   pd->contained = eina_list_remove(pd->contained, obj);

   EINA_COW_WRITE_BEGIN(evas_object_events_cow, obj->events, Evas_Object_Events_Data, events)
     events->parent = NULL;
   EINA_COW_WRITE_END(evas_object_events_cow, obj->events, events);
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_canvas_group_group_change(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_canvas_group_group_calculate(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_canvas_group_group_need_recalculate_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED, Eina_Bool set EINA_UNUSED)
{}

EOLIAN static Eina_Bool
_efl_canvas_object_event_grabber_efl_canvas_group_group_need_recalculate_get(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_gfx_position_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, int x, int y)
{
   efl_gfx_position_set(efl_super(eo_obj, MY_CLASS), x, y);
   efl_gfx_position_set(pd->rect, x, y);
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_gfx_size_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, int w, int h)
{
   efl_gfx_size_set(efl_super(eo_obj, MY_CLASS), w, h);
   efl_gfx_size_set(pd->rect, w, h);
}

EOLIAN static Eina_Bool
_efl_canvas_object_event_grabber_efl_gfx_visible_get(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd)
{
   return pd->vis;
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_gfx_visible_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eina_Bool set)
{
   if (set)
     {
        Evas_Object_Protected_Data *obj;
        Eina_List *l;

        EINA_LIST_FOREACH(pd->contained, l, obj)
          if (obj->object != pd->rect) _stacking_verify(pd, obj);
     }
   pd->vis = !!set;
   efl_gfx_visible_set(pd->rect, set);
   if (pd->restack && pd->freeze && (!set))
     _full_restack(pd);
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_gfx_stack_layer_set(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd, short l)
{
   efl_gfx_stack_layer_set(efl_super(eo_obj, MY_CLASS), l);
   efl_gfx_stack_layer_set(pd->rect, l);
}

static void
_efl_canvas_object_event_grabber_restack(void *data, const Efl_Event *event)
{
   Efl_Object_Event_Grabber_Data *pd = data;

   evas_object_layer_set(pd->rect, evas_object_layer_get(event->object));
   evas_object_stack_below(pd->rect, event->object);

   pd->restack = 1;
   if (pd->vis && pd->freeze) return;
   _full_restack(pd);
}

EOLIAN static Eo *
_efl_canvas_object_event_grabber_efl_object_constructor(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;

   eo_obj = efl_constructor(efl_super(eo_obj, MY_CLASS));
   efl_canvas_object_type_set(eo_obj, MY_CLASS_NAME_LEGACY);
   obj = efl_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   obj->is_event_parent = 1;
   obj->is_smart = 0;

   efl_event_callback_add(eo_obj, EFL_GFX_EVENT_RESTACK, _efl_canvas_object_event_grabber_restack, pd);
   pd->rect = evas_object_rectangle_add(efl_parent_get(eo_obj));
   evas_object_pointer_mode_set(pd->rect, EVAS_OBJECT_POINTER_MODE_NOGRAB);
   efl_parent_set(pd->rect, eo_obj);
   efl_canvas_group_member_add(eo_obj, pd->rect);
   evas_object_color_set(pd->rect, 0, 0, 0, 0);
   return eo_obj;
}

EOLIAN static void
_efl_canvas_object_event_grabber_efl_object_destructor(Eo *eo_obj, Efl_Object_Event_Grabber_Data *pd)
{
   Evas_Object_Protected_Data *obj;
   Eina_List *l, *ln;

   EINA_LIST_FOREACH_SAFE(pd->contained, l, ln, obj)
     efl_canvas_group_member_del(eo_obj, obj->object);
   efl_canvas_group_del(eo_obj);
   efl_destructor(efl_super(eo_obj, MY_CLASS));
}

static void
_efl_canvas_object_event_grabber_class_constructor(Efl_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

const Eina_List *
evas_object_event_grabber_members_list(const Eo *eo_obj)
{
   Efl_Object_Event_Grabber_Data *pd = efl_data_scope_get(eo_obj, MY_CLASS);
   return pd->contained;
}

EOLIAN static void
_efl_canvas_object_event_grabber_freeze_when_visible_set(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd, Eina_Bool set)
{
   set = !!set;
   if (pd->freeze == set) return;
   pd->freeze = set;
   if (pd->vis && pd->restack && (!set))
     _full_restack(pd);
}

EOLIAN static Eina_Bool
_efl_canvas_object_event_grabber_freeze_when_visible_get(Eo *eo_obj EINA_UNUSED, Efl_Object_Event_Grabber_Data *pd)
{
   return pd->freeze;
}

EAPI Evas_Object *
evas_object_event_grabber_add(Evas *eo_e)
{
   EINA_SAFETY_ON_FALSE_RETURN_VAL(efl_isa(eo_e, EVAS_CANVAS_CLASS), NULL);
   return efl_add(MY_CLASS, eo_e, efl_canvas_object_legacy_ctor(efl_added));
}

#include "efl_canvas_object_event_grabber.eo.c"
