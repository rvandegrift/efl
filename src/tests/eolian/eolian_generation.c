#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_EVIL
#include "Evil.h"
#endif

#include <Eina.h>
#include <Eolian.h>

#include "eolian_suite.h"

static Eina_Bool
_files_compare (const char *ref_filename, const char *tmp_filename, const char *ext)
{
   Eina_Bool result = EINA_FALSE;

   FILE *tmp_file = NULL, *ref_file = NULL;
   char *tmp_content = NULL, *ref_content = NULL;

   char ifnbuf[PATH_MAX];
   snprintf(ifnbuf, sizeof(ifnbuf), "%s.%s", tmp_filename, ext);

   tmp_file = fopen(ifnbuf, "rb");
   if (!tmp_file)
     {
        printf("Unable to open %s\n", ifnbuf);
        goto end;
     }
   ref_file = fopen(ref_filename, "rb");
   if (!ref_file)
     {
        printf("Unable to open %s\n", ref_filename);
        goto end;
     }

   fseek(tmp_file, 0, SEEK_END);
   long tmp_filesize = ftell(tmp_file);
   if (tmp_filesize < 0) goto end;
   fseek(tmp_file, 0, SEEK_SET);
   tmp_content = malloc(tmp_filesize + 1);
   if (fread(tmp_content, tmp_filesize, 1, tmp_file) != 1)
     goto end;
   tmp_content[tmp_filesize] = '\0';
   while (tmp_content[tmp_filesize - 1] == '\n')
     tmp_content[--tmp_filesize] = '\0';

   fseek(ref_file, 0, SEEK_END);
   long ref_filesize = ftell(ref_file);
   if (ref_filesize < 0) goto end;
   fseek(ref_file, 0, SEEK_SET);
   ref_content = malloc(ref_filesize + 1);
   if (fread(ref_content, ref_filesize, 1, ref_file) != 1)
     goto end;
   ref_content[ref_filesize] = '\0';
   while (ref_content[ref_filesize - 1] == '\n')
     ref_content[--ref_filesize] = '\0';

   if (tmp_filesize != ref_filesize) goto end;

   result = !memcmp(ref_content, tmp_content, ref_filesize);
end:
   if (tmp_file) fclose(tmp_file);
   if (ref_file) fclose(ref_file);
   if (tmp_content) free(tmp_content);
   if (ref_content) free(ref_content);

   return result;
}

static void
_remove_ref(const char *base, const char *ext)
{
   char ifnbuf[PATH_MAX];
   snprintf(ifnbuf, sizeof(ifnbuf), "%s.%s", base, ext);
   remove(ifnbuf);
}

static int
_eolian_gen_execute(const char *eo_filename, const char *options, const char *output_filename)
{
   char command[PATH_MAX];
   snprintf(command, PATH_MAX,
         EOLIAN_GEN" %s -I \""PACKAGE_DATA_DIR"/data\" -o %s %s",
         options, output_filename, eo_filename);
   return system(command);
}

START_TEST(eolian_dev_impl_code)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_object_impl",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "c");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/object_impl.eo", "-gi", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/object_impl_ref.c", output_filepath, "c"));
   /* Check that nothing is added */
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/object_impl.eo", "-gi", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/object_impl_ref.c", output_filepath, "c"));
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/object_impl_add.eo", "-gi", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/object_impl_add_ref.c", output_filepath, "c"));
}
END_TEST

START_TEST(eolian_types_generation)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_typedef",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "h");
   _remove_ref(output_filepath, "stub.h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/typedef.eo", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/typedef_ref.c", output_filepath, "h"));
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/struct.eo", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/struct_ref.c", output_filepath, "h"));

   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/typedef.eo", "-gs", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/typedef_ref_stub.c", output_filepath, "stub.h"));
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/struct.eo", "-gs", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/struct_ref_stub.c", output_filepath, "stub.h"));
}
END_TEST

START_TEST(eolian_default_values_generation)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_class_simple",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "c");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/class_simple.eo", "-gc", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/class_simple_ref.c", output_filepath, "c"));
}
END_TEST

START_TEST(eolian_override_generation)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_override",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "c");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/override.eo", "-gc", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/override_ref.c", output_filepath, "c"));
}
END_TEST

START_TEST(eolian_functions_descriptions)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_class_simple",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/class_simple.eo", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/class_simple_ref_eo.h", output_filepath, "h"));
   _remove_ref(output_filepath, "legacy.h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/class_simple.eo", "-gl", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/class_simple_ref_legacy.h", output_filepath, "legacy.h"));
}
END_TEST

START_TEST(eolian_import)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_import_types",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/import_types.eot", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/import_types_ref.h", output_filepath, "h"));
}
END_TEST

START_TEST(eolian_docs)
{
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_docs",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/docs.eo", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/docs_ref.h", output_filepath, "h"));
   _remove_ref(output_filepath, "legacy.h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/docs.eo", "-gl", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/docs_ref_legacy.h", output_filepath, "legacy.h"));
}
END_TEST

START_TEST(eolian_function_pointers)
{

   // .eot
   char output_filepath[PATH_MAX] = "";
   snprintf(output_filepath, PATH_MAX, "%s/eolian_function_pointers",
            eina_environment_tmp_get());
   _remove_ref(output_filepath, "h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/function_types.eot", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/function_types_ref.h", output_filepath, "h"));

   // .eo.h
   _remove_ref(output_filepath, "h");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/function_as_argument.eo", "-gh", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/function_as_argument_ref.h", output_filepath, "h"));

   // .eo.c
   _remove_ref(output_filepath, "c");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/function_as_argument.eo", "-gc", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/function_as_argument_ref.c", output_filepath, "c"));

   // .eo.imp.c
   _remove_ref(output_filepath, "c");
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/function_as_argument.eo", "-gi", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/function_as_argument_impl_ref.c", output_filepath, "c"));
   /* Check that nothing is added */
   fail_if(0 != _eolian_gen_execute(PACKAGE_DATA_DIR"/data/function_as_argument.eo", "-gi", output_filepath));
   fail_if(!_files_compare(PACKAGE_DATA_DIR"/data/function_as_argument_impl_ref.c", output_filepath, "c"));
}
END_TEST

void eolian_generation_test(TCase *tc)
{
   tcase_add_test(tc, eolian_types_generation);
   tcase_add_test(tc, eolian_default_values_generation);
   tcase_add_test(tc, eolian_override_generation);
   tcase_add_test(tc, eolian_dev_impl_code);
   tcase_add_test(tc, eolian_functions_descriptions);
   tcase_add_test(tc, eolian_import);
   tcase_add_test(tc, eolian_docs);
   tcase_add_test(tc, eolian_function_pointers);
}
