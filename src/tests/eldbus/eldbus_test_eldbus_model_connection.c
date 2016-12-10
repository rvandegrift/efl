#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdbool.h>

#include <Eina.h>
#include <Ecore.h>
#include <Eldbus_Model.h>

#include "eldbus_test_eldbus_model.h"
#include "eldbus_suite.h"

static Eo *connection = NULL;

#define UNIQUE_NAME_PROPERTY "unique_name"

static void
_setup(void)
{
   check_init();
   connection = create_connection();
}

static void
_teardown(void)
{
   eo_unref(connection);
   check_shutdown();
}

START_TEST(properties_get)
{
   const Eina_Array *properties = NULL;
   properties = efl_model_properties_get(connection);
   ck_assert_ptr_ne(NULL, properties);

   const unsigned int expected_properties_count = 1;
   unsigned int actual_properties_count = eina_array_count(properties);
   ck_assert_int_eq(expected_properties_count, actual_properties_count);
}
END_TEST

START_TEST(property_get)
{
   Eina_Promise *promise;
   promise = efl_model_property_get(connection, UNIQUE_NAME_PROPERTY);
   efl_model_promise_then(promise);

   // Nonexistent property must raise ERROR
   promise = NULL;
   promise = efl_model_property_get(connection, "nonexistent");
   check_efl_model_promise_error(promise, &EFL_MODEL_ERROR_NOT_FOUND);
}
END_TEST

START_TEST(property_set)
{
   Eina_Value value;
   Eina_Promise *promise;

   // Nonexistent property must raise ERROR
   eina_value_setup(&value, EINA_VALUE_TYPE_INT);
   eina_value_set(&value, 1);
   efl_model_property_set(connection, "nonexistent", &value, &promise);
   check_efl_model_promise_error(promise, &EFL_MODEL_ERROR_NOT_FOUND);

   // UNIQUE_NAME_PROPERTY is read-only
   promise = NULL;
   efl_model_property_set(connection, UNIQUE_NAME_PROPERTY, &value, &promise);
   check_efl_model_promise_error(promise, &EFL_MODEL_ERROR_READ_ONLY);

   eina_value_flush(&value);
}
END_TEST

static void
_test_children_count(Eo *efl_model)
{
   // At least this connection <unique_name> and 'org.freedesktop.DBus' must exist
   check_efl_model_children_count_ge(efl_model, 2);
}

START_TEST(children_count)
{
   _test_children_count(connection);
}
END_TEST

START_TEST(children_slice_get)
{
   check_efl_model_children_slice_get(connection);
}
END_TEST

START_TEST(child_add)
{
   Eo *child;
   child = efl_model_child_add(connection);
   ck_assert_ptr_eq(NULL, child);
}
END_TEST

START_TEST(child_del)
{
   unsigned int expected_children_count = 0;
   Eina_Promise *promise;
   promise = efl_model_children_count_get(connection);
   ck_assert_ptr_ne(NULL, promise);
   expected_children_count = efl_model_promise_then_u(promise);

   Eo *child = efl_model_first_child_get(connection);
   efl_model_child_del(connection, child);

   unsigned int actual_children_count = 0;
   promise = efl_model_children_count_get(connection);
   actual_children_count = efl_model_promise_then_u(promise);

   ck_assert_int_le(expected_children_count, actual_children_count);
}
END_TEST

void eldbus_test_eldbus_model_connection(TCase *tc)
{
   tcase_add_checked_fixture(tc, _setup, _teardown);
   tcase_add_test(tc, properties_get);
   tcase_add_test(tc, property_get);
   tcase_add_test(tc, property_set);
   tcase_add_test(tc, children_count);
   tcase_add_test(tc, children_slice_get);
   tcase_add_test(tc, child_add);
   tcase_add_test(tc, child_del);
}
