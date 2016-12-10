#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eo.h>

#include "eo_suite.h"
#include "eo_test_class_simple.h"

START_TEST(eo_test_simple)
{
   fail_if(!eo_init()); /* one init by test suite */
   fail_if(!eo_init());
   fail_if(!eo_shutdown());
   fail_if(eo_shutdown());
}
END_TEST

START_TEST(eo_test_init_shutdown)
{
   Eo *obj;

   fail_if(!eo_init());
   ck_assert_str_eq("Eo_Base", eo_class_name_get(EO_BASE_CLASS));

   /* XXX-1: Essential for the next test to assign the wrong op. */
   obj = eo_add(SIMPLE_CLASS, NULL);
   simple_a_set(obj, 1);
   ck_assert_int_eq(1, simple_a_get(obj));

   /* XXX-1: Essential for the next test to cache the op. */
   ck_assert_int_eq(0xBEEF, simple2_class_beef_get(SIMPLE2_CLASS));
   eo_unref(obj);
   fail_if(eo_shutdown());

   fail_if(!eo_init());
   ck_assert_str_eq("Eo_Base", eo_class_name_get(EO_BASE_CLASS));

   /* XXX-1: Verify that the op was not cached. */
   ck_assert_int_eq(0xBEEF, simple2_class_beef_get(SIMPLE2_CLASS));
   fail_if(eo_shutdown());
}
END_TEST

void eo_test_init(TCase *tc)
{
   tcase_add_test(tc, eo_test_simple);
   tcase_add_test(tc, eo_test_init_shutdown);
}
