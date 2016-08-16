#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <Eina.h>
#include <Eolian.h>

#include "eolian_suite.h"

START_TEST(eolian_namespaces)
{
   const Eolian_Class *class11, *class112, *class21, *class_no, *impl_class;
   const Eolian_Function *fid;
   Eina_Iterator *iter;
   Eolian_Function_Type func_type;
   const char *class_name, *val1, *val2;
   const Eolian_Implement *impl;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/nmsp1_class1.eo"));

   /* Classes existence  */
   fail_if(!(class11 = eolian_class_get_by_name("nmsp1.class1")));
   fail_if(!(class112 = eolian_class_get_by_name("nmsp1.nmsp11.class2")));
   fail_if(!(class21 = eolian_class_get_by_name("nmsp2.class1")));
   fail_if(!(class_no = eolian_class_get_by_name("no_nmsp")));

   /* Check names and namespaces*/
   fail_if(strcmp(eolian_class_name_get(class11), "class1"));
   fail_if(!(iter = eolian_class_namespaces_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp1"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class112), "class2"));
   fail_if(!(iter = eolian_class_namespaces_get(class112)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(!(eina_iterator_next(iter, (void**)&val2)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp1"));
   fail_if(strcmp(val2, "nmsp11"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class21), "class1"));
   fail_if(!(iter = eolian_class_namespaces_get(class21)));
   fail_if(!(eina_iterator_next(iter, (void**)&val1)));
   fail_if(eina_iterator_next(iter, &dummy));
   fail_if(strcmp(val1, "nmsp2"));
   eina_iterator_free(iter);

   fail_if(strcmp(eolian_class_name_get(class_no), "no_nmsp"));
   fail_if(eolian_class_namespaces_get(class_no));

   /* Inherits */
   fail_if(!(iter = eolian_class_inherits_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_get_by_name(class_name) != class112);
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_get_by_name(class_name) != class21);
   fail_if(!(eina_iterator_next(iter, (void**)&class_name)));
   fail_if(eolian_class_get_by_name(class_name) != class_no);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* Implements */
   fail_if(!(iter = eolian_class_implements_get(class11)));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(fid = eolian_implement_function_get(impl, &func_type)));
   fail_if(impl_class != class112);
   fail_if(strcmp(eolian_function_name_get(fid), "a"));
   fail_if(func_type != EOLIAN_PROP_SET);

   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(fid = eolian_implement_function_get(impl, &func_type)));
   fail_if(impl_class != class_no);
   fail_if(strcmp(eolian_function_name_get(fid), "foo"));
   fail_if(func_type != EOLIAN_METHOD);
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* Virtual regression */
   fail_if(!(fid = eolian_class_function_get_by_name(class112, "a", EOLIAN_UNRESOLVED)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_PROP_SET));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_events)
{
   const Eolian_Class *class;
   Eina_Iterator *iter;
   const char *name, *type_name;
   const Eolian_Type *type;
   const Eolian_Event *ev;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/events.eo"));

   /* Class */
   fail_if(!(class = eolian_class_get_by_name("Events")));
   fail_if(strcmp(eolian_class_event_prefix_get(class), "totally_not_events"));

   /* Events */
   fail_if(!(iter = eolian_class_events_get(class)));
   /* Clicked */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!(name = eolian_event_name_get(ev)));
   fail_if(eolian_event_type_get(ev));
   fail_if(strcmp(name, "clicked"));
   fail_if(!eolian_event_is_beta(ev));
   fail_if(eolian_event_is_hot(ev));
   fail_if(eolian_event_is_restart(ev));
   /* Clicked,double */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!(name = eolian_event_name_get(ev)));
   fail_if(!(type = eolian_event_type_get(ev)));
   fail_if(strcmp(name, "clicked,double"));
   type_name = eolian_type_name_get(type);
   fail_if(strcmp(type_name, "Evas_Event_Clicked_Double_Info"));
   fail_if(eolian_event_is_beta(ev));
   fail_if(eolian_event_is_hot(ev));
   fail_if(eolian_event_is_restart(ev));
   /* Hot */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!(name = eolian_event_name_get(ev)));
   fail_if(eolian_event_type_get(ev));
   fail_if(strcmp(name, "hot"));
   fail_if(eolian_event_is_beta(ev));
   fail_if(!eolian_event_is_hot(ev));
   fail_if(eolian_event_is_restart(ev));
   /* Restart */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!(name = eolian_event_name_get(ev)));
   fail_if(eolian_event_type_get(ev));
   fail_if(strcmp(name, "restart"));
   fail_if(eolian_event_is_beta(ev));
   fail_if(eolian_event_is_hot(ev));
   fail_if(!eolian_event_is_restart(ev));
   /* Hot Restart */
   fail_if(!(eina_iterator_next(iter, (void**)&ev)));
   fail_if(!(name = eolian_event_name_get(ev)));
   fail_if(eolian_event_type_get(ev));
   fail_if(strcmp(name, "hot_restart"));
   fail_if(eolian_event_is_beta(ev));
   fail_if(!eolian_event_is_hot(ev));
   fail_if(!eolian_event_is_restart(ev));

   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   /* Check eolian_class_event_get_by_name */
   fail_if(!eolian_class_event_get_by_name(class, "clicked,double"));
   fail_if(eolian_class_event_get_by_name(class, "clicked,triple"));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_override)
{
   Eina_Iterator *iter;
   const Eolian_Function *fid = NULL;
   const Eolian_Class *impl_class = NULL;
   const Eolian_Function *impl_func = NULL;
   const Eolian_Class *class, *base;
   const Eolian_Implement *impl;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/override.eo"));

   /* Class */
   fail_if(!(class = eolian_class_get_by_name("Override")));
   fail_if(!(base = eolian_class_get_by_name("Base")));

   /* Base ctor */
   fail_if(!(fid = eolian_class_function_get_by_name(base, "constructor", EOLIAN_UNRESOLVED)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_UNRESOLVED));
   fail_if(!eolian_function_is_implemented(fid, EOLIAN_UNRESOLVED, class));
   fail_if(!eolian_function_is_implemented(fid, EOLIAN_METHOD, class));
   fail_if(eolian_function_is_implemented(fid, EOLIAN_PROP_GET, class));

   /* Property */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_PROP_SET));
   fail_if(eolian_function_is_virtual_pure(fid, EOLIAN_PROP_GET));
   fail_if(eolian_function_is_implemented(fid, EOLIAN_PROP_SET, class));
   fail_if(!eolian_function_is_implemented(fid, EOLIAN_PROP_GET, class));
   fail_if(eolian_function_is_implemented(fid, EOLIAN_PROPERTY, class));

   /* Method */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_virtual_pure(fid, EOLIAN_METHOD));
   fail_if(eolian_function_is_implemented(fid, EOLIAN_UNRESOLVED, class));
   fail_if(eolian_function_is_implemented(fid, EOLIAN_UNRESOLVED, base));

   fail_if(!(fid = eolian_class_function_get_by_name(base, "z", EOLIAN_PROPERTY)));
   fail_if(!eolian_function_is_implemented(fid, EOLIAN_PROPERTY, class));
   fail_if(!eolian_function_is_implemented(fid, EOLIAN_PROP_SET, class));

   /* Implements */
   fail_if(!(iter = eolian_class_implements_get(class)));

   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(eolian_implement_is_auto(impl));
   fail_if(eolian_implement_is_empty(impl));
   fail_if(eolian_implement_is_virtual(impl));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(impl_func = eolian_implement_function_get(impl, NULL)));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "constructor"));

   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!eolian_implement_is_auto(impl));
   fail_if(eolian_implement_is_empty(impl));
   fail_if(eolian_implement_is_virtual(impl));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(impl_func = eolian_implement_function_get(impl, NULL)));
   fail_if(impl_class != class);
   fail_if(strcmp(eolian_function_name_get(impl_func), "b"));
   fail_if(!eolian_function_is_auto(impl_func, EOLIAN_PROP_SET));
   fail_if(eolian_function_is_auto(impl_func, EOLIAN_PROP_GET));
   fail_if(eolian_function_is_empty(impl_func, EOLIAN_PROP_SET));
   fail_if(eolian_function_is_empty(impl_func, EOLIAN_PROP_GET));
   fail_if(eolian_function_is_virtual_pure(impl_func, EOLIAN_PROP_SET));
   fail_if(eolian_function_is_virtual_pure(impl_func, EOLIAN_PROP_GET));

   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(eolian_implement_is_auto(impl));
   fail_if(!eolian_implement_is_empty(impl));
   fail_if(eolian_implement_is_virtual(impl));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(impl_func = eolian_implement_function_get(impl, NULL)));
   fail_if(impl_class != class);
   fail_if(strcmp(eolian_function_name_get(impl_func), "bar"));
   fail_if(eolian_function_is_auto(impl_func, EOLIAN_METHOD));
   fail_if(!eolian_function_is_empty(impl_func, EOLIAN_METHOD));
   fail_if(eolian_function_is_virtual_pure(impl_func, EOLIAN_METHOD));

   eina_iterator_free(iter);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_consts)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Class *class;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/consts.eo"));
   fail_if(!(class = eolian_class_get_by_name("Consts")));

   /* Method */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(EINA_FALSE == eolian_function_object_is_const(fid));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_ctor_dtor)
{
   Eina_Iterator *iter;
   const Eolian_Class *impl_class = NULL;
   const Eolian_Function *impl_func = NULL;
   const Eolian_Class *class, *base;
   const Eolian_Implement *impl;
   const Eolian_Constructor *ctor;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/ctor_dtor.eo"));
   fail_if(!(class = eolian_class_get_by_name("Ctor_Dtor")));
   fail_if(!(base = eolian_class_get_by_name("Base")));

   /* Class ctor/dtor */
   fail_if(!eolian_class_ctor_enable_get(class));
   fail_if(!eolian_class_dtor_enable_get(class));

   /* Base ctor/dtor */
   fail_if(!(iter = eolian_class_implements_get(class)));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(impl_func = eolian_implement_function_get(impl, NULL)));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "constructor"));
   fail_if(!eolian_function_is_constructor(impl_func, base));
   fail_if(!eolian_function_is_constructor(impl_func, class));
   fail_if(!(eina_iterator_next(iter, (void**)&impl)));
   fail_if(!(impl_class = eolian_implement_class_get(impl)));
   fail_if(!(impl_func = eolian_implement_function_get(impl, NULL)));
   fail_if(impl_class != base);
   fail_if(strcmp(eolian_function_name_get(impl_func), "destructor"));
   fail_if(eolian_function_is_constructor(impl_func, base));
   fail_if(eolian_function_is_constructor(impl_func, class));
   eina_iterator_free(iter);

   /* Custom ctors/dtors */
   fail_if(!eolian_class_function_get_by_name(base, "destructor", EOLIAN_METHOD));
   fail_if(!(iter = eolian_class_constructors_get(class)));
   fail_if(!(eina_iterator_next(iter, (void**)&ctor)));
   fail_if(eolian_constructor_is_optional(ctor));
   fail_if(!(impl_class = eolian_constructor_class_get(ctor)));
   fail_if(!(impl_func = eolian_constructor_function_get(ctor)));
   fail_if(impl_class != class);
   fail_if(strcmp(eolian_function_name_get(impl_func), "custom_constructor_1"));
   fail_if(!eolian_function_is_constructor(impl_func, class));
   fail_if(eolian_function_is_constructor(impl_func, base));
   fail_if(!(eina_iterator_next(iter, (void**)&ctor)));
   fail_if(!eolian_constructor_is_optional(ctor));
   fail_if(!(impl_class = eolian_constructor_class_get(ctor)));
   fail_if(!(impl_func = eolian_constructor_function_get(ctor)));
   fail_if(impl_class != class);
   fail_if(strcmp(eolian_function_name_get(impl_func), "custom_constructor_2"));
   fail_if(!eolian_function_is_constructor(impl_func, class));
   fail_if(eolian_function_is_constructor(impl_func, base));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_typedef)
{
   const Eolian_Type *type = NULL;
   const Eolian_Typedecl *tdl = NULL;
   const char *type_name = NULL;
   Eina_Iterator *iter = NULL;
   const Eolian_Class *class;
   const char *file;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/typedef.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Typedef")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   /* Basic type */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Evas.Coord")));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_ALIAS);
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(strcmp(type_name, "Coord"));
   fail_if(!(type_name = eolian_typedecl_c_type_get(tdl)));
   fail_if(strcmp(type_name, "typedef int Evas_Coord"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_typedecl_base_type_get(tdl)));
   fail_if(!(type_name = eolian_type_name_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(eolian_type_is_const(type));
   fail_if(eolian_type_base_type_get(type));
   fail_if(strcmp(type_name, "int"));

   /* File */
   fail_if(!(file = eolian_typedecl_file_get(tdl)));
   fail_if(strcmp(file, "typedef.eo"));

   /* Lowest alias base */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Evas.Coord3")));
   fail_if(!(type = eolian_typedecl_aliased_base_get(tdl)));
   fail_if(strcmp(eolian_type_name_get(type), "int"));

   /* Complex type */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("List_Objects")));
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(strcmp(type_name, "List_Objects"));
   fail_if(!(type = eolian_typedecl_base_type_get(tdl)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(!!eolian_type_next_type_get(type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(strcmp(type_name, "Eo *"));
   fail_if(eolian_type_is_own(type));
   eina_stringshare_del(type_name);

   /* List */
   fail_if(!(iter = eolian_typedecl_aliases_get_by_file("typedef.eo")));
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(strcmp(type_name, "Coord"));
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(strcmp(type_name, "List_Objects"));
   /* coord2 and coord3, skip */
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   /* not generated extern, skip */
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   /* not generated undefined type, skip */
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   /* event type, tested by generation tests */
   fail_if(!eina_iterator_next(iter, (void**)&tdl));
   fail_if(eina_iterator_next(iter, (void**)&tdl));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_complex_type)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Function_Parameter *param = NULL;
   const Eolian_Type *type = NULL;
   const char *type_name = NULL;
   Eina_Iterator *iter = NULL;
   const Eolian_Class *class;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/complex_type.eo"));
   fail_if(!(class = eolian_class_get_by_name("Complex_Type")));

   /* Properties return type */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!(type = eolian_function_return_type_get(fid, EOLIAN_PROP_SET)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(!!eolian_type_next_type_get(type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_Array *"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(!!eolian_type_next_type_get(type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eo **"));
   eina_stringshare_del(type_name);
   /* Properties parameter type */
   fail_if(!(iter = eolian_property_values_get(fid, EOLIAN_PROP_GET)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   fail_if(strcmp(eolian_parameter_name_get(param), "value"));
   fail_if(!(type = eolian_parameter_type_get(param)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(!!eolian_type_next_type_get(type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "int"));
   eina_stringshare_del(type_name);

   /* Methods return type */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!(type = eolian_function_return_type_get(fid, EOLIAN_METHOD)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_List *"));
   eina_stringshare_del(type_name);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(!!eolian_type_next_type_get(type));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(eolian_type_is_own(type));
   fail_if(strcmp(type_name, "Eina_Stringshare *"));
   eina_stringshare_del(type_name);
   /* Methods parameter type */
   fail_if(!(iter = eolian_function_parameters_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   fail_if(strcmp(eolian_parameter_name_get(param), "buf"));
   fail_if(!(type = eolian_parameter_type_get(param)));
   fail_if(!(type_name = eolian_type_c_type_get(type)));
   fail_if(!eolian_type_is_own(type));
   fail_if(strcmp(type_name, "char *"));
   eina_stringshare_del(type_name);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_scope)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Class *class;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/scope.eo"));
   fail_if(!(class = eolian_class_get_by_name("Scope")));

   /* Property scope */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_PROPERTY) != EOLIAN_SCOPE_PROTECTED);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "b", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_PROPERTY) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "c", EOLIAN_PROPERTY)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_PROP_GET) != EOLIAN_SCOPE_PUBLIC);
   fail_if(eolian_function_scope_get(fid, EOLIAN_PROP_SET) != EOLIAN_SCOPE_PROTECTED);

   /* Method scope */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "bar", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PROTECTED);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foobar", EOLIAN_METHOD)));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PUBLIC);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_simple_parsing)
{
   const Eolian_Function *fid = NULL;
   const char *string = NULL, *ptype = NULL;
   const Eolian_Function_Parameter *param = NULL;
   const Eolian_Expression *expr = NULL;
   const Eolian_Class *class;
   const Eolian_Type *tp;
   Eina_Iterator *iter;
   Eolian_Value v;
   void *dummy;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/class_simple.eo"));
   fail_if(!(class = eolian_class_get_by_name("Class_Simple")));
   fail_if(eolian_class_get_by_file("class_simple.eo") != class);
   fail_if(strcmp(eolian_class_file_get(class), "class_simple.eo"));

   /* Class */
   fail_if(eolian_class_type_get(class) != EOLIAN_CLASS_REGULAR);
   fail_if(eolian_class_inherits_get(class) != NULL);
   fail_if(strcmp(eolian_class_legacy_prefix_get(class), "evas_object_simple"));
   fail_if(strcmp(eolian_class_eo_prefix_get(class), "efl_canvas_object_simple"));
   fail_if(strcmp(eolian_class_data_type_get(class), "Evas_Simple_Data"));

   /* c get func */
   fail_if(!(string = eolian_class_c_get_function_name_get(class)));
   fail_if(strcmp(string, "class_simple_class_get"));
   eina_stringshare_del(string);

   /* Property */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(strcmp(eolian_function_name_get(fid), "a"));
   fail_if(!eolian_function_is_beta(fid));
   fail_if(eolian_function_class_get(fid) != class);
   /* Set return */
   tp = eolian_function_return_type_get(fid, EOLIAN_PROP_SET);
   fail_if(!tp);
   fail_if(strcmp(eolian_type_name_get(tp), "bool"));
   expr = eolian_function_return_default_value_get(fid, EOLIAN_PROP_SET);
   fail_if(!expr);
   v = eolian_expression_eval(expr, EOLIAN_MASK_BOOL);
   fail_if(v.type != EOLIAN_EXPR_BOOL);
   /* Get return */
   tp = eolian_function_return_type_get(fid, EOLIAN_PROP_GET);
   fail_if(tp);

   /* Function parameters */
   fail_if(eolian_property_keys_get(fid, EOLIAN_PROP_GET) != NULL);
   fail_if(!(iter = eolian_property_values_get(fid, EOLIAN_PROP_GET)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);
   fail_if(strcmp(eolian_type_name_get(eolian_parameter_type_get(param)), "int"));
   fail_if(strcmp(eolian_parameter_name_get(param), "value"));
   expr = eolian_parameter_default_value_get(param);
   fail_if(!expr);
   v = eolian_expression_eval(expr, EOLIAN_MASK_INT);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 100);

   /* legacy only + c only */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "b", EOLIAN_PROPERTY)));
   fail_if(eolian_function_is_legacy_only(fid, EOLIAN_PROP_GET));
   fail_if(!eolian_function_is_legacy_only(fid, EOLIAN_PROP_SET));
   fail_if(!eolian_function_is_c_only(fid));
   fail_if(eolian_function_is_beta(fid));

   /* Method */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_beta(fid));
   fail_if(eolian_type_is_ref(eolian_function_return_type_get(fid, EOLIAN_METHOD)));
   /* Function return */
   tp = eolian_function_return_type_get(fid, EOLIAN_METHOD);
   fail_if(!tp);
   string = eolian_type_c_type_get(tp);
   fail_if(!string);
   fail_if(strcmp(string, "char *"));
   eina_stringshare_del(string);
   expr = eolian_function_return_default_value_get(fid, EOLIAN_METHOD);
   fail_if(!expr);
   v = eolian_expression_eval(expr, EOLIAN_MASK_NULL);
   fail_if(v.type != EOLIAN_EXPR_NULL);
   fail_if(eolian_function_is_legacy_only(fid, EOLIAN_METHOD));

   /* Function parameters */
   fail_if(!(iter = eolian_function_parameters_get(fid)));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eolian_parameter_direction_get(param) != EOLIAN_IN_PARAM);
   fail_if(strcmp(eolian_type_name_get(eolian_parameter_type_get(param)), "int"));
   fail_if(strcmp(eolian_parameter_name_get(param), "a"));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eolian_parameter_direction_get(param) != EOLIAN_INOUT_PARAM);
   ptype = eolian_type_name_get(eolian_parameter_type_get(param));
   fail_if(strcmp(ptype, "char"));
   fail_if(strcmp(eolian_parameter_name_get(param), "b"));
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eolian_parameter_direction_get(param) != EOLIAN_OUT_PARAM);
   fail_if(strcmp(eolian_type_name_get(eolian_parameter_type_get(param)), "double"));
   fail_if(strcmp(eolian_parameter_name_get(param), "c"));
   expr = eolian_parameter_default_value_get(param);
   fail_if(!expr);
   v = eolian_expression_eval(expr, EOLIAN_MASK_FLOAT);
   fail_if(v.type != EOLIAN_EXPR_DOUBLE);
   fail_if(v.value.d != 1337.6);
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(eolian_parameter_direction_get(param) != EOLIAN_IN_PARAM);
   fail_if(strcmp(eolian_type_name_get(eolian_parameter_type_get(param)), "int"));
   fail_if(!eolian_type_is_ref(eolian_parameter_type_get(param)));
   fail_if(strcmp(eolian_parameter_name_get(param), "d"));
   fail_if(eina_iterator_next(iter, &dummy));
   eina_iterator_free(iter);

   /* legacy only + c only */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "bar", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_legacy_only(fid, EOLIAN_METHOD));
   fail_if(!eolian_function_is_c_only(fid));
   fail_if(eolian_function_is_beta(fid));
   fail_if(!eolian_type_is_ref(eolian_function_return_type_get(fid, EOLIAN_METHOD)));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_struct)
{
   const Eolian_Struct_Type_Field *field = NULL;
   const Eolian_Type *type = NULL, *ftype = NULL;
   const Eolian_Typedecl *tdl = NULL;
   const Eolian_Class *class;
   const Eolian_Function *func;
   const char *type_name;
   const char *file;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/struct.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Struct")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   /* named struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Named")));
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(!(file = eolian_typedecl_file_get(tdl)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_STRUCT);
   fail_if(strcmp(type_name, "Named"));
   fail_if(strcmp(file, "struct.eo"));
   fail_if(!(field = eolian_typedecl_struct_field_get(tdl, "field")));
   fail_if(!(ftype = eolian_typedecl_struct_field_type_get(field)));
   fail_if(!eolian_type_is_ref(ftype));
   fail_if(!(type_name = eolian_type_name_get(ftype)));
   fail_if(strcmp(type_name, "int"));
   fail_if(!(field = eolian_typedecl_struct_field_get(tdl, "something")));
   fail_if(!(ftype = eolian_typedecl_struct_field_type_get(field)));
   fail_if(eolian_type_is_ref(ftype));
   fail_if(!(type_name = eolian_type_c_type_get(ftype)));
   fail_if(strcmp(type_name, "const char *"));
   eina_stringshare_del(type_name);
   fail_if(!(field = eolian_typedecl_struct_field_get(tdl, "arr")));
   fail_if(!(ftype = eolian_typedecl_struct_field_type_get(field)));
   fail_if(eolian_type_is_ref(ftype));
   fail_if(eolian_type_array_size_get(ftype) != 16);
   fail_if(eolian_type_type_get(ftype) != EOLIAN_TYPE_STATIC_ARRAY);
   fail_if(!(type_name = eolian_type_c_type_get(ftype)));
   fail_if(strcmp(type_name, "int *"));
   eina_stringshare_del(type_name);
   fail_if(!(field = eolian_typedecl_struct_field_get(tdl, "tarr")));
   fail_if(!(ftype = eolian_typedecl_struct_field_type_get(field)));
   fail_if(eolian_type_is_ref(ftype));
   fail_if(!(type_name = eolian_type_c_type_get(ftype)));
   fail_if(eolian_type_type_get(ftype) != EOLIAN_TYPE_TERMINATED_ARRAY);
   fail_if(strcmp(type_name, "const char **"));
   eina_stringshare_del(type_name);

   /* referencing */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Another")));
   fail_if(!(type_name = eolian_typedecl_name_get(tdl)));
   fail_if(!(file = eolian_typedecl_file_get(tdl)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_STRUCT);
   fail_if(strcmp(type_name, "Another"));
   fail_if(strcmp(file, "struct.eo"));
   fail_if(!(field = eolian_typedecl_struct_field_get(tdl, "field")));
   fail_if(!(ftype = eolian_typedecl_struct_field_type_get(field)));
   fail_if(!(type_name = eolian_type_name_get(ftype)));
   fail_if(strcmp(type_name, "Named"));
   fail_if(eolian_type_type_get(ftype) != EOLIAN_TYPE_REGULAR);
   fail_if(eolian_typedecl_type_get(eolian_type_typedecl_get(ftype))
       != EOLIAN_TYPEDECL_STRUCT);

   /* opaque struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Opaque")));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_STRUCT_OPAQUE);

   /* use in function */
   fail_if(!(func = eolian_class_function_get_by_name(class, "bar", EOLIAN_METHOD)));
   fail_if(!(type = eolian_function_return_type_get(func, EOLIAN_METHOD)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_POINTER);
   fail_if(!(type = eolian_type_base_type_get(type)));
   fail_if(eolian_type_type_get(type) != EOLIAN_TYPE_REGULAR);
   fail_if(!(tdl = eolian_type_typedecl_get(type)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_STRUCT);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_extern)
{
   const Eolian_Typedecl *tdl = NULL;
   const Eolian_Class *class;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/extern.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Extern")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   /* regular type */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Foo")));
   fail_if(eolian_typedecl_is_extern(tdl));

   /* extern type */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Evas.Coord")));
   fail_if(!eolian_typedecl_is_extern(tdl));

   /* regular struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("X")));
   fail_if(eolian_typedecl_is_extern(tdl));

   /* extern struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Y")));
   fail_if(!eolian_typedecl_is_extern(tdl));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_var)
{
   const Eolian_Variable *var = NULL;
   const Eolian_Expression *exp = NULL;
   const Eolian_Type *type = NULL;
   const Eolian_Class *class;
   Eolian_Value v;
   const char *name;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/var.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Var")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   /* regular constant */
   fail_if(!(var = eolian_variable_constant_get_by_name("Foo")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_CONSTANT);
   fail_if(eolian_variable_is_extern(var));
   fail_if(!(type = eolian_variable_base_type_get(var)));
   fail_if(!(name = eolian_type_name_get(type)));
   fail_if(strcmp(name, "int"));
   fail_if(!(exp = eolian_variable_value_get(var)));
   v = eolian_expression_eval_type(exp, type);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 5);

   /* regular global */
   fail_if(!(var = eolian_variable_global_get_by_name("Bar")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_GLOBAL);
   fail_if(eolian_variable_is_extern(var));
   fail_if(!(type = eolian_variable_base_type_get(var)));
   fail_if(!(name = eolian_type_name_get(type)));
   fail_if(strcmp(name, "float"));
   fail_if(!(exp = eolian_variable_value_get(var)));
   v = eolian_expression_eval_type(exp, type);
   fail_if(v.type != EOLIAN_EXPR_FLOAT);
   fail_if(((int)v.value.f) != 10);

   /* no-value global */
   fail_if(!(var = eolian_variable_global_get_by_name("Baz")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_GLOBAL);
   fail_if(eolian_variable_is_extern(var));
   fail_if(!(type = eolian_variable_base_type_get(var)));
   fail_if(!(name = eolian_type_name_get(type)));
   fail_if(strcmp(name, "long"));
   fail_if(eolian_variable_value_get(var));

   /* extern global  */
   fail_if(!(var = eolian_variable_global_get_by_name("Bah")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_GLOBAL);
   fail_if(!eolian_variable_is_extern(var));
   fail_if(!(type = eolian_variable_base_type_get(var)));
   fail_if(!(name = eolian_type_name_get(type)));
   fail_if(strcmp(name, "double"));
   fail_if(eolian_variable_value_get(var));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_enum)
{
   const Eolian_Enum_Type_Field *field = NULL;
   const Eolian_Variable *var = NULL;
   const Eolian_Typedecl *tdl = NULL;
   const Eolian_Type *type = NULL;
   const Eolian_Class *class;
   const Eolian_Expression *exp;
   Eina_Stringshare *cname;
   const char *name;
   Eolian_Value v;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/enum.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Enum")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   fail_if(!(tdl = eolian_typedecl_enum_get_by_name("Foo")));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "first")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 0);

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "bar")));
   fail_if(eolian_typedecl_enum_field_value_get(field, EINA_FALSE));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "baz")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 15);

   fail_if(!(tdl = eolian_typedecl_enum_get_by_name("Bar")));
   fail_if(strcmp(eolian_typedecl_enum_legacy_prefix_get(tdl), "test"));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "foo")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 15);

   cname = eolian_typedecl_enum_field_c_name_get(field);
   fail_if(strcmp(cname, "TEST_FOO"));
   eina_stringshare_del(cname);

   fail_if(!(tdl = eolian_typedecl_enum_get_by_name("Baz")));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "flag1")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != (1 << 0));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "flag2")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != (1 << 1));

   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "flag3")));
   fail_if(!(exp = eolian_typedecl_enum_field_value_get(field, EINA_FALSE)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != (1 << 2));

   fail_if(!(tdl = eolian_typedecl_enum_get_by_name("Name.Spaced")));
   fail_if(!(field = eolian_typedecl_enum_field_get(tdl, "pants")));

   cname = eolian_typedecl_enum_field_c_name_get(field);
   fail_if(strcmp(cname, "NAME_SPACED_PANTS"));
   eina_stringshare_del(cname);

   fail_if(!(var = eolian_variable_constant_get_by_name("Bah")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_CONSTANT);
   fail_if(eolian_variable_is_extern(var));
   fail_if(!(type = eolian_variable_base_type_get(var)));
   fail_if(!(name = eolian_type_name_get(type)));
   fail_if(strcmp(name, "Baz"));
   fail_if(!(exp = eolian_variable_value_get(var)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != (1 << 0));

   fail_if(!(var = eolian_variable_constant_get_by_name("Pants")));
   fail_if(eolian_variable_type_get(var) != EOLIAN_VAR_CONSTANT);
   fail_if(!(exp = eolian_variable_value_get(var)));
   v = eolian_expression_eval(exp, EOLIAN_MASK_ALL);
   fail_if(v.type != EOLIAN_EXPR_INT);
   fail_if(v.value.i != 5);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_class_funcs)
{
   const Eolian_Function *fid = NULL;
   const Eolian_Class *class;

   eolian_init();
   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/class_funcs.eo"));
   fail_if(!(class = eolian_class_get_by_name("Class_Funcs")));

   /* Class properties */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "a", EOLIAN_PROPERTY)));
   fail_if(!eolian_function_is_class(fid));
   fail_if(!(fid = eolian_class_function_get_by_name(class, "b", EOLIAN_PROPERTY)));
   fail_if(eolian_function_is_class(fid));

   /* Class methods */
   fail_if(!(fid = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_class(fid));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "bar", EOLIAN_METHOD)));
   fail_if(eolian_function_is_class(fid));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PUBLIC);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "baz", EOLIAN_METHOD)));
   fail_if(!eolian_function_is_class(fid));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PROTECTED);
   fail_if(!(fid = eolian_class_function_get_by_name(class, "bah", EOLIAN_METHOD)));
   fail_if(eolian_function_is_class(fid));
   fail_if(eolian_function_scope_get(fid, EOLIAN_METHOD) != EOLIAN_SCOPE_PROTECTED);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_free_func)
{
   const Eolian_Class *class;
   const Eolian_Typedecl *tdl;
   const Eolian_Type *type;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/free_func.eo"));

   /* Check that the class Dummy is still readable */
   fail_if(!(class = eolian_class_get_by_name("Free_Func")));
   fail_if(!eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD));

   /* regular struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Named1")));
   fail_if(eolian_typedecl_free_func_get(tdl));
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Named2")));
   fail_if(strcmp(eolian_typedecl_free_func_get(tdl), "test_free"));

   /* typedef */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Typedef1")));
   fail_if(eolian_typedecl_free_func_get(tdl));
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Typedef2")));
   fail_if(strcmp(eolian_typedecl_free_func_get(tdl), "def_free"));

   /* opaque struct */
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Opaque1")));
   fail_if(eolian_typedecl_free_func_get(tdl));
   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Opaque2")));
   fail_if(strcmp(eolian_typedecl_free_func_get(tdl), "opaque_free"));

   /* pointer */
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Pointer1")));
   fail_if(!(type = eolian_typedecl_base_type_get(tdl)));
   fail_if(eolian_type_free_func_get(type));
   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Pointer2")));
   fail_if(!(type = eolian_typedecl_base_type_get(tdl)));
   fail_if(strcmp(eolian_type_free_func_get(type), "ptr_free"));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_null)
{
   const Eolian_Class *class;
   const Eolian_Function *func;
   const Eolian_Function_Parameter *param;
   Eina_Iterator *iter;

   eolian_init();

   /* Parsing */
   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/null.eo"));

   fail_if(!(class = eolian_class_get_by_name("Null")));
   fail_if(!(func = eolian_class_function_get_by_name(class, "foo", EOLIAN_METHOD)));

   fail_if(!(iter = eolian_function_parameters_get(func)));

   /* no qualifiers */
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(strcmp(eolian_parameter_name_get(param), "x"));
   fail_if(eolian_parameter_is_nullable(param));
   fail_if(eolian_parameter_is_optional(param));

   /* nullable */
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(strcmp(eolian_parameter_name_get(param), "y"));
   fail_if(!eolian_parameter_is_nullable(param));
   fail_if(eolian_parameter_is_optional(param));

   /* optional */
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(strcmp(eolian_parameter_name_get(param), "z"));
   fail_if(eolian_parameter_is_nullable(param));
   fail_if(!eolian_parameter_is_optional(param));

   /* both */
   fail_if(!(eina_iterator_next(iter, (void**)&param)));
   fail_if(strcmp(eolian_parameter_name_get(param), "w"));
   fail_if(!eolian_parameter_is_nullable(param));
   fail_if(!eolian_parameter_is_optional(param));

   fail_if(eina_iterator_next(iter, (void**)&param));
   eina_iterator_free(iter);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_import)
{
   const Eolian_Class *class;
   const Eolian_Typedecl *tdl;

   eolian_init();

   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));

   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/import.eo"));
   fail_if(!(class = eolian_class_get_by_name("Import")));

   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Imported")));
   fail_if(strcmp(eolian_typedecl_file_get(tdl), "import_types.eot"));

   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Imported_Struct")));
   fail_if(strcmp(eolian_typedecl_file_get(tdl), "import_types.eot"));

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_decl)
{
   const Eolian_Declaration *decl;
   const Eolian_Typedecl *tdl;
   const Eolian_Class *class;
   const Eolian_Variable *var;
   Eina_Iterator *itr;

   eolian_init();

   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));

   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/decl.eo"));
   fail_if(!(class = eolian_class_get_by_name("Decl")));

   fail_if(!(itr = eolian_declarations_get_by_file("decl.eo")));

   fail_if(!eina_iterator_next(itr, (void**)&decl));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_STRUCT);
   fail_if(strcmp(eolian_declaration_name_get(decl), "A"));
   fail_if(!(tdl = eolian_declaration_data_type_get(decl)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_STRUCT);
   fail_if(strcmp(eolian_typedecl_name_get(tdl), "A"));

   fail_if(!eina_iterator_next(itr, (void**)&decl));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_ENUM);
   fail_if(strcmp(eolian_declaration_name_get(decl), "B"));
   fail_if(!(tdl = eolian_declaration_data_type_get(decl)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_ENUM);
   fail_if(strcmp(eolian_typedecl_name_get(tdl), "B"));

   fail_if(!eina_iterator_next(itr, (void**)&decl));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_ALIAS);
   fail_if(strcmp(eolian_declaration_name_get(decl), "C"));
   fail_if(!(tdl = eolian_declaration_data_type_get(decl)));
   fail_if(eolian_typedecl_type_get(tdl) != EOLIAN_TYPEDECL_ALIAS);
   fail_if(strcmp(eolian_typedecl_name_get(tdl), "C"));

   fail_if(!eina_iterator_next(itr, (void**)&decl));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_VAR);
   fail_if(strcmp(eolian_declaration_name_get(decl), "pants"));
   fail_if(!(var = eolian_declaration_variable_get(decl)));
   fail_if(strcmp(eolian_variable_name_get(var), "pants"));

   fail_if(!eina_iterator_next(itr, (void**)&decl));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_CLASS);
   fail_if(strcmp(eolian_declaration_name_get(decl), "Decl"));
   fail_if(eolian_declaration_class_get(decl) != class);

   fail_if(eina_iterator_next(itr, (void**)&decl));

   fail_if(!(decl = eolian_declaration_get_by_name("pants")));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_VAR);

   fail_if(!(decl = eolian_declaration_get_by_name("A")));
   fail_if(eolian_declaration_type_get(decl) != EOLIAN_DECL_STRUCT);

   eolian_shutdown();
}
END_TEST

START_TEST(eolian_docs)
{
   const Eolian_Typedecl *tdl;
   const Eolian_Class *class;
   const Eolian_Event *event;
   const Eolian_Variable *var;
   const Eolian_Function *fid;
   const Eolian_Documentation *doc;
   const Eolian_Function_Parameter *par;
   const Eolian_Struct_Type_Field *sfl;
   const Eolian_Enum_Type_Field *efl;
   Eina_Iterator *itr;

   eolian_init();

   fail_if(!eolian_directory_scan(PACKAGE_DATA_DIR"/data"));

   fail_if(!eolian_file_parse(PACKAGE_DATA_DIR"/data/docs.eo"));

   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Foo")));
   fail_if(!(doc = eolian_typedecl_documentation_get(tdl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "This is struct Foo. It does stuff."));
   fail_if(strcmp(eolian_documentation_description_get(doc),
                  "Note: This is a note.\n\n"
                  "This is a longer description for struct Foo.\n\n"
                  "Warning: This is a warning. You can only use Warning: "
                  "and Note: at the beginning of a paragraph.\n\n"
                  "This is another paragraph."));
   fail_if(strcmp(eolian_documentation_since_get(doc),
                  "1.66"));

   fail_if(!(sfl = eolian_typedecl_struct_field_get(tdl, "field1")));
   fail_if(!(doc = eolian_typedecl_struct_field_documentation_get(sfl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Field documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(sfl = eolian_typedecl_struct_field_get(tdl, "field2")));
   fail_if(eolian_typedecl_struct_field_documentation_get(sfl));

   fail_if(!(sfl = eolian_typedecl_struct_field_get(tdl, "field3")));
   fail_if(!(doc = eolian_typedecl_struct_field_documentation_get(sfl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Another field documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(tdl = eolian_typedecl_enum_get_by_name("Bar")));
   fail_if(!(doc = eolian_typedecl_documentation_get(tdl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for enum Bar."));
   fail_if(eolian_documentation_description_get(doc));
   fail_if(eolian_documentation_since_get(doc));

   fail_if(!(efl = eolian_typedecl_enum_field_get(tdl, "blah")));
   fail_if(eolian_typedecl_enum_field_documentation_get(efl));

   fail_if(!(efl = eolian_typedecl_enum_field_get(tdl, "foo")));
   fail_if(!(doc = eolian_typedecl_enum_field_documentation_get(efl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for foo."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(efl = eolian_typedecl_enum_field_get(tdl, "bar")));
   fail_if(!(doc = eolian_typedecl_enum_field_documentation_get(efl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for bar."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(tdl = eolian_typedecl_alias_get_by_name("Alias")));
   fail_if(!(doc = eolian_typedecl_documentation_get(tdl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for typedef."));
   fail_if(strcmp(eolian_documentation_description_get(doc),
                  "More docs for typedef. See @Bar."));
   fail_if(strcmp(eolian_documentation_since_get(doc),
                  "2.0"));

   fail_if(!(var = eolian_variable_global_get_by_name("pants")));
   fail_if(!(doc = eolian_variable_documentation_get(var)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for var."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(tdl = eolian_typedecl_struct_get_by_name("Opaque")));
   fail_if(!(doc = eolian_typedecl_documentation_get(tdl)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Opaque struct docs. See @Foo for another struct."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(class = eolian_class_get_by_name("Docs")));
   fail_if(!(doc = eolian_class_documentation_get(class)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Docs for class."));
   fail_if(strcmp(eolian_documentation_description_get(doc),
                  "More docs for class. Testing references now. "
                  "@Foo @Bar @Alias @pants @Docs.meth @Docs.prop "
                  "@Docs.prop.get @Docs.prop.set @Foo.field1 @Bar.foo @Docs"));

   fail_if(!(fid = eolian_class_function_get_by_name(class, "meth", EOLIAN_METHOD)));
   fail_if(!(doc = eolian_function_documentation_get(fid, EOLIAN_METHOD)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Method documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(itr = eolian_function_parameters_get(fid)));

   fail_if(!eina_iterator_next(itr, (void**)&par));
   fail_if(!(doc = eolian_parameter_documentation_get(par)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Param documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!eina_iterator_next(itr, (void**)&par));
   fail_if(eolian_parameter_documentation_get(par));

   fail_if(!eina_iterator_next(itr, (void**)&par));
   fail_if(!(doc = eolian_parameter_documentation_get(par)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Another param documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(eina_iterator_next(itr, (void**)&par));
   eina_iterator_free(itr);

   fail_if(!(doc = eolian_function_return_documentation_get(fid, EOLIAN_METHOD)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Return documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(fid = eolian_class_function_get_by_name(class, "prop", EOLIAN_PROPERTY)));
   fail_if(!(doc = eolian_function_documentation_get(fid, EOLIAN_PROPERTY)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Property common documentation."));
   fail_if(eolian_documentation_description_get(doc));
   fail_if(strcmp(eolian_documentation_since_get(doc),
                  "1.18"));
   fail_if(!(doc = eolian_function_documentation_get(fid, EOLIAN_PROP_GET)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Get documentation."));
   fail_if(eolian_documentation_description_get(doc));
   fail_if(!(doc = eolian_function_documentation_get(fid, EOLIAN_PROP_SET)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Set documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(!(itr = eolian_property_values_get(fid, EOLIAN_PROP_GET)));

   fail_if(!eina_iterator_next(itr, (void**)&par));
   fail_if(!(doc = eolian_parameter_documentation_get(par)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Value documentation."));
   fail_if(eolian_documentation_description_get(doc));

   fail_if(eina_iterator_next(itr, (void**)&par));
   eina_iterator_free(itr);

   fail_if(!(event = eolian_class_event_get_by_name(class, "clicked")));
   fail_if(!(doc = eolian_event_documentation_get(event)));
   fail_if(strcmp(eolian_documentation_summary_get(doc),
                  "Event docs."));
   fail_if(eolian_documentation_description_get(doc));

   eolian_shutdown();
}
END_TEST

void eolian_parsing_test(TCase *tc)
{
   tcase_add_test(tc, eolian_simple_parsing);
   tcase_add_test(tc, eolian_ctor_dtor);
   tcase_add_test(tc, eolian_scope);
   tcase_add_test(tc, eolian_complex_type);
   tcase_add_test(tc, eolian_typedef);
   tcase_add_test(tc, eolian_consts);
   tcase_add_test(tc, eolian_override);
   tcase_add_test(tc, eolian_events);
   tcase_add_test(tc, eolian_namespaces);
   tcase_add_test(tc, eolian_struct);
   tcase_add_test(tc, eolian_extern);
   tcase_add_test(tc, eolian_var);
   tcase_add_test(tc, eolian_enum);
   tcase_add_test(tc, eolian_class_funcs);
   tcase_add_test(tc, eolian_free_func);
   tcase_add_test(tc, eolian_null);
   tcase_add_test(tc, eolian_import);
   tcase_add_test(tc, eolian_decl);
   tcase_add_test(tc, eolian_docs);
}
