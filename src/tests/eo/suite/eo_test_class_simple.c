#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "eo_test_class_simple.h"

#define MY_CLASS SIMPLE_CLASS

EAPI const Eo_Event_Description _EV_A_CHANGED =
        EO_EVENT_DESCRIPTION("a,changed");

EAPI const Eo_Event_Description _EV_A_CHANGED2 =
        EO_EVENT_DESCRIPTION("a,changed");

static void
_a_set(Eo *obj EINA_UNUSED, void *class_data, int a)
{
   Simple_Public_Data *pd = class_data;
   printf("%s %d\n", eo_class_name_get(MY_CLASS), a);
   pd->a = a;

   eo_event_callback_call(obj, EV_A_CHANGED, &pd->a);
}

static int
_a_get(Eo *obj EINA_UNUSED, void *class_data)
{
   Simple_Public_Data *pd = class_data;

   return pd->a;
}

static Eina_Bool
_a_print(Eo *obj EINA_UNUSED, void *class_data)
{
   const Simple_Public_Data *pd = class_data;
   printf("Print %s %d\n", eo_class_name_get(MY_CLASS), pd->a);

   return EINA_TRUE;
}

static Eina_Bool
_class_hi_print(Eo_Class *klass, void *data EINA_UNUSED)
{
   printf("Hi Print %s\n", eo_class_name_get(klass));

   return EINA_TRUE;
}

EO_FUNC_BODYV(simple_part_get, Eo *, NULL, EO_FUNC_CALL(name), const char *name);

static Eo *
_part_get(Eo *obj, void *class_data EINA_UNUSED, const char *name EINA_UNUSED)
{
   /* A normal part get will do something saner, we just create objects. */
   return eo_add(SIMPLE_CLASS, obj);
}

EO_VOID_FUNC_BODYV(simple_recursive, EO_FUNC_CALL(n), int n);

static void
_recursive(Eo *obj, void *class_data EINA_UNUSED, int n)
{
   static int count = 0;

   if (count < n)
     {
        count++;
        simple_recursive(obj, n);
     }
   else
     count = 0;
}

static void
_dbg_info_get(Eo *eo_obj, void *_pd EINA_UNUSED, Eo_Dbg_Info *root)
{
   eo_dbg_info_get(eo_super(eo_obj, MY_CLASS), root);
   Eo_Dbg_Info *group = EO_DBG_INFO_LIST_APPEND(root, "Test list");
   EO_DBG_INFO_APPEND(group, "Test", EINA_VALUE_TYPE_INT, 8);
}

EO_VOID_FUNC_BODYV(simple_a_set, EO_FUNC_CALL(a), int a);
EO_FUNC_BODY(simple_a_get, int, 0);
EO_FUNC_BODY(simple_a_print, Eina_Bool, EINA_FALSE);
EO_FUNC_BODY_CONST(simple_class_hi_print, Eina_Bool, EINA_FALSE);
EO_VOID_FUNC_BODY(simple_pure_virtual);
EO_VOID_FUNC_BODY(simple_no_implementation);

static Eo_Op_Description op_descs[] = {
     EO_OP_FUNC(simple_a_set, _a_set),
     EO_OP_FUNC(simple_a_get, _a_get),
     EO_OP_FUNC(simple_a_print, _a_print),
     EO_OP_CLASS_FUNC(simple_class_hi_print, _class_hi_print),
     EO_OP_FUNC(simple_recursive, _recursive),
     EO_OP_FUNC(simple_part_get, _part_get),
     EO_OP_FUNC(simple_pure_virtual, NULL),
     EO_OP_FUNC_OVERRIDE(eo_dbg_info_get, _dbg_info_get),
};

static const Eo_Class_Description class_desc = {
     EO_VERSION,
     "Simple",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(op_descs),
     NULL,
     sizeof(Simple_Public_Data),
     NULL,
     NULL
};

EO_DEFINE_CLASS(simple_class_get, &class_desc, EO_CLASS, NULL)


static int
_beef_get(Eo *obj EINA_UNUSED, void *class_data EINA_UNUSED)
{
   return 0xBEEF;
}

EO_FUNC_BODY_CONST(simple2_class_beef_get, int, 0);

static Eo_Op_Description op_descs2[] = {
     EO_OP_CLASS_FUNC(simple2_class_beef_get, _beef_get),
};

static const Eo_Class_Description class_desc2 = {
     EO_VERSION,
     "Simple2",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(op_descs2),
     NULL,
     0,
     NULL,
     NULL
};

EO_DEFINE_CLASS(simple2_class_get, &class_desc2, EO_CLASS, NULL)

static Eo_Base*
_interface_get(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, const Eo_Base *klass)
{
   if (klass == SEARCHABLE_CLASS) return obj;

   return eo_provider_find(eo_super(obj, SEARCHABLE_CLASS), klass);
}

static Eo_Op_Description op_descs_searchable[] = {
     EO_OP_FUNC_OVERRIDE(eo_provider_find, _interface_get)
};

static const Eo_Class_Description class_desc_searchable = {
     EO_VERSION,
     "Searchable",
     EO_CLASS_TYPE_REGULAR,
     EO_CLASS_DESCRIPTION_OPS(op_descs_searchable),
     NULL,
     0,
     NULL,
     NULL
};

EO_DEFINE_CLASS(searchable_class_get, &class_desc_searchable, EO_CLASS, NULL)