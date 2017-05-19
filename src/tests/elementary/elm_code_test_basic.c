#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERNAL_API_ARGESFSDFEFC

#include <stdlib.h>

#include "elm_suite.h"
#include "Elementary.h"

START_TEST (elm_code_create_test)
{
   Elm_Code *code;

   elm_init(1, NULL);
   code = elm_code_create();

   ck_assert(!!code);
   ck_assert(elm_code_file_path_get(code->file) == NULL);
   elm_code_free(code);
   elm_shutdown();
}
END_TEST

START_TEST (elm_code_open_test)
{
   char *path = TESTS_SRC_DIR "/testfile.txt";
   char realpath1[PATH_MAX], realpath2[PATH_MAX];
   Elm_Code *code;

   elm_init(1, NULL);
   code = elm_code_create();
   elm_code_file_open(code, path);

   realpath(path, realpath1);
   realpath(elm_code_file_path_get(code->file), realpath2);
   ck_assert(!!code);
   ck_assert_str_eq(realpath1, realpath2);
   elm_code_free(code);
   elm_shutdown();
}
END_TEST


void elm_code_test_basic(TCase *tc)
{
   tcase_add_test(tc, elm_code_create_test);
   tcase_add_test(tc, elm_code_open_test);
}
