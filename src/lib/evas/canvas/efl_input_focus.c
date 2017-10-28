#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#define EFL_INPUT_EVENT_PROTECTED

#include <Evas.h>

#include <Evas_Internal.h>

#define EFL_INTERNAL_UNSTABLE
#include "interfaces/efl_common_internal.h"

#define MY_CLASS EFL_INPUT_FOCUS_CLASS

static Efl_Input_Focus *s_cached_event = NULL;

static void
_del_hook(Eo *evt)
{
   if (!s_cached_event)
     {
        if (efl_parent_get(evt))
          {
             efl_ref(evt);
             efl_parent_set(evt, NULL);
          }
        efl_reuse(evt);
        s_cached_event = evt;
        efl_input_reset(s_cached_event);
     }
   else
     {
        efl_del_intercept_set(evt, NULL);
        efl_del(evt);
     }
}

static void
_efl_input_focus_free(Efl_Input_Focus_Data *pd)
{
   efl_wref_del_safe(&pd->object_wref);
   efl_unref(pd->device);
}

EOLIAN static Efl_Object *
_efl_input_focus_efl_object_constructor(Eo *obj,
                                        Efl_Input_Focus_Data *pd EINA_UNUSED)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));
   return obj;
}

EOLIAN static void
_efl_input_focus_efl_object_destructor(Eo *obj,
                                       Efl_Input_Focus_Data *pd)
{
   _efl_input_focus_free(pd);
   efl_destructor(efl_super(obj, MY_CLASS));
}

EOLIAN static void
_efl_input_focus_class_destructor(Efl_Class *klass EINA_UNUSED)
{
   // this is a strange situation...
   efl_del_intercept_set(s_cached_event, NULL);
   efl_del(s_cached_event);
   s_cached_event = NULL;
}

EOLIAN static void
_efl_input_focus_object_set(Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd,
                            Efl_Object *object)
{
   pd->object_wref = object;
}

EOLIAN static Efl_Object *
_efl_input_focus_object_get(Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd)
{
   return pd->object_wref;
}

EOLIAN static void
_efl_input_focus_efl_input_event_device_set(Eo *obj EINA_UNUSED,
                                            Efl_Input_Focus_Data *pd,
                                            Efl_Input_Device *device)
{
   efl_replace(&pd->device, device);
}

EOLIAN static Efl_Input_Device *
_efl_input_focus_efl_input_event_device_get(Eo *obj EINA_UNUSED,
                                            Efl_Input_Focus_Data *pd)
{
   return pd->device;
}

EOLIAN static void
_efl_input_focus_efl_input_event_timestamp_set(Eo *obj EINA_UNUSED,
                                               Efl_Input_Focus_Data *pd,
                                               double ms)
{
   pd->timestamp = ms;
}

EOLIAN static double
_efl_input_focus_efl_input_event_timestamp_get(Eo *obj EINA_UNUSED,
                                               Efl_Input_Focus_Data *pd)
{
   return pd->timestamp;
}

EOLIAN static Efl_Input_Focus *
_efl_input_focus_efl_input_event_dup(Eo *obj EINA_UNUSED, Efl_Input_Focus_Data *pd)
{
   Efl_Input_Focus_Data *ev;
   Efl_Input_Focus *evt;

   evt = efl_add(MY_CLASS, NULL);
   ev = efl_data_scope_get(evt, MY_CLASS);
   if (!ev) return NULL;

   memcpy(ev, pd, sizeof(*ev));
   ev->eo        = evt;
   ev->device    = efl_ref(pd->device);
   efl_wref_add(ev->object_wref, &ev->object_wref);

   return evt;
}

EOLIAN static Efl_Input_Focus *
_efl_input_focus_efl_input_event_instance_get(Eo *klass EINA_UNUSED, void *_pd EINA_UNUSED,
                                              Eo *owner, void **priv)
{
   Efl_Input_Focus_Data *ev;
   Efl_Input_Focus *evt;

   if (s_cached_event)
     {
        evt = s_cached_event;
        s_cached_event = NULL;
        efl_parent_set(evt, owner);
     }
   else
     {
        evt = efl_add(MY_CLASS, owner);
        efl_del_intercept_set(evt, _del_hook);
     }

   ev = efl_data_scope_get(evt, MY_CLASS);
   if (priv) *priv = ev;
   return evt;
}

EOLIAN static void
_efl_input_focus_efl_input_event_reset(Eo *obj, Efl_Input_Focus_Data *pd)
{
   _efl_input_focus_free(pd);
   memset(pd, 0, sizeof(Efl_Input_Focus_Data));
   pd->eo = obj;
}

/* Internal EO APIs */

#define EFL_INPUT_FOCUS_EXTRA_CLASS_OPS \
   EFL_OBJECT_OP_FUNC(efl_input_instance_get, _efl_input_focus_efl_input_event_instance_get)

#include "efl_input_focus.eo.c"
