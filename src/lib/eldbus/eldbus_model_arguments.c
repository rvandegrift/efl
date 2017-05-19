#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eldbus_model_arguments_private.h"
#include "eldbus_model_private.h"

#include <Ecore.h>
#include <Eina.h>
#include <Eldbus.h>

#define MY_CLASS ELDBUS_MODEL_ARGUMENTS_CLASS
#define MY_CLASS_NAME "Eldbus_Model_Arguments"

#define ARGUMENT_FORMAT "arg%u"

static void _eldbus_model_arguments_properties_load(Eldbus_Model_Arguments_Data *);
static void _eldbus_model_arguments_unload(Eldbus_Model_Arguments_Data *);
static Eina_Bool _eldbus_model_arguments_is_input_argument(Eldbus_Model_Arguments_Data *, const char *);
static Eina_Bool _eldbus_model_arguments_is_output_argument(Eldbus_Model_Arguments_Data *, const char *);
static Eina_Bool _eldbus_model_arguments_property_set(Eldbus_Model_Arguments_Data *, Eina_Value *, const char *);
static unsigned int _eldbus_model_arguments_argument_index_get(Eldbus_Model_Arguments_Data *, const char *);

static void
_eldbus_model_arguments_hash_free(Eina_Value *value)
{
   eina_value_free(value);
}

static Efl_Object*
_eldbus_model_arguments_efl_object_constructor(Eo *obj, Eldbus_Model_Arguments_Data *pd)
{
   obj = efl_constructor(efl_super(obj, MY_CLASS));

   pd->obj = obj;
   pd->properties_array = NULL;
   pd->properties_hash = eina_hash_string_superfast_new(EINA_FREE_CB(_eldbus_model_arguments_hash_free));
   pd->pending_list = NULL;
   pd->proxy = NULL;
   pd->arguments = NULL;
   pd->name = NULL;
   return obj;
}

static void
_eldbus_model_arguments_constructor(Eo *obj EINA_UNUSED,
                                    Eldbus_Model_Arguments_Data *pd,
                                    Eldbus_Proxy *proxy,
                                    const char *name,
                                    const Eina_List *arguments)
{
   EINA_SAFETY_ON_NULL_RETURN(proxy);
   EINA_SAFETY_ON_NULL_RETURN(name);

   pd->proxy = eldbus_proxy_ref(proxy);
   pd->arguments = arguments;
   pd->name = eina_stringshare_add(name);
}

static void
_eldbus_model_arguments_efl_object_destructor(Eo *obj, Eldbus_Model_Arguments_Data *pd)
{
   _eldbus_model_arguments_unload(pd);

   eina_hash_free(pd->properties_hash);

   eina_stringshare_del(pd->name);
   eldbus_proxy_unref(pd->proxy);

   efl_destructor(efl_super(obj, MY_CLASS));
}

static Eina_Array const *
_eldbus_model_arguments_efl_model_properties_get(Eo *obj EINA_UNUSED,
                                                      Eldbus_Model_Arguments_Data *pd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(pd, NULL);

   _eldbus_model_arguments_properties_load(pd);
   return pd->properties_array;
}

static void
_eldbus_model_arguments_properties_load(Eldbus_Model_Arguments_Data *pd)
{
   unsigned int arguments_count;
   unsigned int i;

   if (pd->properties_array)
     return;

   arguments_count = eina_list_count(pd->arguments);

   pd->properties_array = eina_array_new(arguments_count);
   EINA_SAFETY_ON_NULL_RETURN(pd->properties_array);

   for (i = 0; i < arguments_count; ++i)
     {
        Eldbus_Introspection_Argument *arg;
        const Eina_Value_Type *type;
        Eina_Stringshare *name;
        Eina_Value *value;

        name = eina_stringshare_printf(ARGUMENT_FORMAT, i);
        if (!name) continue;

        eina_array_push(pd->properties_array, name);

        arg = eina_list_nth(pd->arguments, i);
        type = _dbus_type_to_eina_value_type(arg->type[0]);
        value = eina_value_new(type);
        eina_hash_add(pd->properties_hash, name, value);
     }
}

static Efl_Future*
_eldbus_model_arguments_efl_model_property_set(Eo *obj EINA_UNUSED,
                                                    Eldbus_Model_Arguments_Data *pd,
                                                    const char *property,
                                               Eina_Value const* value)
{
   Eina_Value *prop_value;
   Eina_Value *promise_value;
   Efl_Promise* promise = efl_add(EFL_PROMISE_CLASS, obj);
   Efl_Future* future = efl_promise_future_get(promise);

   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(property, promise, EFL_MODEL_ERROR_INCORRECT_VALUE, future);
   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(value, promise, EFL_MODEL_ERROR_INCORRECT_VALUE, future);
   DBG("(%p): property=%s", obj, property);

   _eldbus_model_arguments_properties_load(pd);

   Eina_Bool ret = _eldbus_model_arguments_is_input_argument(pd, property);
   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(ret, promise, EFL_MODEL_ERROR_READ_ONLY, future);

   prop_value = eina_hash_find(pd->properties_hash, property);
   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(prop_value, promise, EFL_MODEL_ERROR_NOT_FOUND, future);

   eina_value_flush(prop_value);
   eina_value_copy(value, prop_value);

   promise_value = eina_value_new(eina_value_type_get(value));
   eina_value_copy(value, promise_value);
   efl_promise_value_set(promise, promise_value, (Eina_Free_Cb)&eina_value_free);
   return future;
}

static Efl_Future*
_eldbus_model_arguments_efl_model_property_get(Eo *obj EINA_UNUSED,
                                                    Eldbus_Model_Arguments_Data *pd,
                                                    const char *property)
{
   Efl_Promise *promise = efl_add(EFL_PROMISE_CLASS, obj);
   Efl_Future *future = efl_promise_future_get(promise);
   Eina_Value *promise_value;

   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(property, promise, EFL_MODEL_ERROR_INCORRECT_VALUE, future);
   DBG("(%p): property=%s", obj, property);

   _eldbus_model_arguments_properties_load(pd);

   Eina_Value* value = eina_hash_find(pd->properties_hash, property);
   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(value, promise, EFL_MODEL_ERROR_NOT_FOUND, future);

   Eina_Bool ret = _eldbus_model_arguments_is_output_argument(pd, property);
   ELDBUS_MODEL_ON_ERROR_EXIT_PROMISE_SET(ret, promise, EFL_MODEL_ERROR_PERMISSION_DENIED, future);

   promise_value = eina_value_new(eina_value_type_get(value));
   eina_value_copy(value, promise_value);
   efl_promise_value_set(promise, promise_value, (Eina_Free_Cb)&eina_value_free);
   return future;
}

static Eo *
_eldbus_model_arguments_efl_model_child_add(Eo *obj EINA_UNUSED, Eldbus_Model_Arguments_Data *pd EINA_UNUSED)
{
   return NULL;
}

static void
_eldbus_model_arguments_efl_model_child_del(Eo *obj EINA_UNUSED,
                                                 Eldbus_Model_Arguments_Data *pd EINA_UNUSED,
                                                 Eo *child EINA_UNUSED)
{
}

static Efl_Future*
_eldbus_model_arguments_efl_model_children_slice_get(Eo *obj EINA_UNUSED,
                                                          Eldbus_Model_Arguments_Data *pd EINA_UNUSED,
                                                          unsigned start EINA_UNUSED,
                                                          unsigned count EINA_UNUSED)
{
   Efl_Promise *promise = efl_add(EFL_PROMISE_CLASS, obj);
   efl_promise_failed_set(promise, EFL_MODEL_ERROR_NOT_SUPPORTED);
   return efl_promise_future_get(promise);
}

static Efl_Future*
_eldbus_model_arguments_efl_model_children_count_get(Eo *obj EINA_UNUSED,
                                                         Eldbus_Model_Arguments_Data *pd EINA_UNUSED)
{
   Efl_Promise *promise = efl_add(EFL_PROMISE_CLASS, obj);
   Efl_Future* future = efl_promise_future_get(promise);
   unsigned *count = malloc(sizeof(unsigned));
   *count = 0;
   efl_promise_value_set(promise, count, free);
   return future;
}

static const char *
_eldbus_model_arguments_name_get(Eo *obj EINA_UNUSED, Eldbus_Model_Arguments_Data *pd)
{
   return pd->name;
}

static void
_eldbus_model_arguments_unload(Eldbus_Model_Arguments_Data *pd)
{
   Eldbus_Pending *pending;

   EINA_SAFETY_ON_NULL_RETURN(pd);

   EINA_LIST_FREE(pd->pending_list, pending)
     eldbus_pending_cancel(pending);

   if (pd->properties_array)
     {
        Eina_Stringshare *property;
        Eina_Array_Iterator it;
        unsigned int i;

        EINA_ARRAY_ITER_NEXT(pd->properties_array, i, property, it)
          eina_stringshare_del(property);
        eina_array_free(pd->properties_array);
        pd->properties_array = NULL;
     }

   eina_hash_free_buckets(pd->properties_hash);
}

Eina_Bool
eldbus_model_arguments_process_arguments(Eldbus_Model_Arguments_Data *pd,
                                         const Eldbus_Message *msg,
                                         Eldbus_Pending *pending)
{
   const Eldbus_Introspection_Argument *argument;
   const char *error_name, *error_text;
   const Eina_List *it;
   Eina_Value *value_struct;
   Eina_Array *changed_properties;
   unsigned int i = 0;
   Eina_Bool result = EINA_FALSE;

   _eldbus_model_arguments_properties_load(pd);

   pd->pending_list = eina_list_remove(pd->pending_list, pending);
   if (eldbus_message_error_get(msg, &error_name, &error_text))
     {
        ERR("%s: %s", error_name, error_text);
        //efl_model_error_notify(pd->obj);
        return EINA_FALSE;
     }

   value_struct = eldbus_message_to_eina_value(msg);
   if (value_struct == NULL)
     {
        INF("%s", "No output arguments");
        return EINA_TRUE;
     }

   changed_properties = eina_array_new(1);

   EINA_LIST_FOREACH(pd->arguments, it, argument)
     {
        if (ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_IN != argument->direction)
          {
             Eina_Stringshare *property;
             Eina_Bool ret;

             property = eina_array_data_get(pd->properties_array, i);
             EINA_SAFETY_ON_NULL_GOTO(property, on_error);

             ret = _eldbus_model_arguments_property_set(pd, value_struct, property);
             EINA_SAFETY_ON_FALSE_GOTO(ret, on_error);

             ret = eina_array_push(changed_properties, property);
             EINA_SAFETY_ON_FALSE_GOTO(ret, on_error);
          }

        ++i;
     }

   if (eina_array_count(changed_properties))
     {
        Efl_Model_Property_Event evt = {.changed_properties = changed_properties};
        efl_event_callback_call(pd->obj, EFL_MODEL_EVENT_PROPERTIES_CHANGED, &evt);
     }

   result = EINA_TRUE;

on_error:
   eina_array_free(changed_properties);
   eina_value_free(value_struct);

   return result;
}

static Eina_Bool
_eldbus_model_arguments_property_set(Eldbus_Model_Arguments_Data *pd,
                                     Eina_Value *value_struct,
                                     const char *property)
{
   Eina_Value *prop_value;
   Eina_Value value;
   Eina_Bool ret;

   _eldbus_model_arguments_properties_load(pd);

   prop_value = eina_hash_find(pd->properties_hash, property);
   EINA_SAFETY_ON_NULL_RETURN_VAL(prop_value, EINA_FALSE);

   ret = eina_value_struct_value_get(value_struct, "arg0", &value);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ret, EINA_FALSE);

   eina_value_flush(prop_value);
   ret = eina_value_copy(&value, prop_value);
   eina_value_flush(&value);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(ret, EINA_FALSE);

   return ret;
}

static Eina_Bool
_eldbus_model_arguments_is(Eldbus_Model_Arguments_Data *pd,
                           const char *argument,
                           Eldbus_Introspection_Argument_Direction direction)
{
   Eldbus_Introspection_Argument *argument_introspection;
   unsigned int i;

   _eldbus_model_arguments_properties_load(pd);

   i = _eldbus_model_arguments_argument_index_get(pd, argument);
   if (i >= eina_array_count(pd->properties_array))
     {
        WRN("Argument not found: %s", argument);
        return false;
     }

   argument_introspection = eina_list_nth(pd->arguments, i);
   EINA_SAFETY_ON_NULL_RETURN_VAL(argument_introspection, EINA_FALSE);

   return argument_introspection->direction == direction;
}

static Eina_Bool
_eldbus_model_arguments_is_input_argument(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   return _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_IN);
}

static Eina_Bool
_eldbus_model_arguments_is_output_argument(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   return _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_OUT) ||
     _eldbus_model_arguments_is(pd, argument, ELDBUS_INTROSPECTION_ARGUMENT_DIRECTION_NONE);
}

static unsigned int
_eldbus_model_arguments_argument_index_get(Eldbus_Model_Arguments_Data *pd, const char *argument)
{
   Eina_Stringshare *name;
   Eina_Array_Iterator it;
   unsigned int i = 0;
   _eldbus_model_arguments_properties_load(pd);

   EINA_ARRAY_ITER_NEXT(pd->properties_array, i, name, it)
     {
        if (strcmp(name, argument) == 0)
          return i;
     }

   return ++i;
}

#include "eldbus_model_arguments.eo.c"
