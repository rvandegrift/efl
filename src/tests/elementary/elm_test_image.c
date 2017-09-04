#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#include <Elementary.h>
#include "elm_suite.h"

static const char pathfmt[] =  ELM_IMAGE_DATA_DIR"/images/icon_%02d.png";
static const char invalid[] = "thereisnosuchimage.png";
#define MAX_IMAGE_ID 23

typedef struct _Test_Data Test_Data;
struct _Test_Data
{
   int image_id;
   int success;
};

START_TEST (elm_atspi_role_get)
{
   Evas_Object *win, *image;
   Elm_Atspi_Role role;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "image", ELM_WIN_BASIC);

   image = elm_image_add(win);
   role = elm_interface_atspi_accessible_role_get(image);

   ck_assert(role == ELM_ATSPI_ROLE_IMAGE);

   elm_shutdown();
}
END_TEST

static void
_async_error_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Test_Data *td = data;
   char path[PATH_MAX];
   sprintf(path, pathfmt, td->image_id);
   elm_image_file_set(obj, path, NULL);

   td->success = 1;
}

static void
_async_opened_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Test_Data *td = data;
   const char *ff, *kk, *r1, *r2;
   char path[PATH_MAX];

   sprintf(path, pathfmt, td->image_id);
   elm_image_file_get(obj, &ff, &kk);
   r1 = strrchr(ff, '/');
   r2 = strrchr(path, '/');
   ck_assert(eina_streq(r1, r2));
   ck_assert(!kk);

   fprintf(stderr, "opened: %s\n", ff);

   if (td->image_id < MAX_IMAGE_ID / 2)
     {
        td->image_id++;
        sprintf(path, pathfmt, td->image_id);
        elm_image_file_set(obj, path, NULL);
        return;
     }
   else if (td->image_id < MAX_IMAGE_ID)
     {
        // mini stress-test
        for (; td->image_id < MAX_IMAGE_ID;)
          {
             sprintf(path, pathfmt, ++td->image_id);
             elm_image_file_set(obj, path, NULL);
          }
        return;
     }

   if (td->success == 1)
     td->success = 2;
}

static void
_async_ready_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Test_Data *td = data;

   const char *ff;
   elm_image_file_get(obj, &ff, 0);
   fprintf(stderr, "ready : %s\n", ff);
   if (td->success == 2)
     {
        td->success = 3;
        ecore_main_loop_quit();
     }
}

static Eina_Bool
_timeout_cb(void *data)
{
   Test_Data *td = data;

   td->success = 0;
   ecore_main_loop_quit();

   return ECORE_CALLBACK_CANCEL;
}

START_TEST (elm_image_async_path)
{
   Evas_Object *win, *image;
   Eina_Bool ok;
   Test_Data td;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "image", ELM_WIN_BASIC);

   td.success = 0;
   td.image_id = 0;

   image = elm_image_add(win);
   elm_image_async_open_set(image, 1);
   evas_object_smart_callback_add(image, "load,error", _async_error_cb, &td);
   evas_object_smart_callback_add(image, "load,open", _async_opened_cb, &td);
   evas_object_smart_callback_add(image, "load,ready", _async_ready_cb, &td);
   evas_object_show(image);
   ok = elm_image_file_set(image, invalid, NULL);
   ck_assert(ok);

   ecore_timer_add(10.0, _timeout_cb, &td);

   elm_run();
   ck_assert(td.success == 3);

   elm_shutdown();
}
END_TEST

START_TEST (elm_image_async_mmap)
{
   Evas_Object *win, *image;
   Eina_Bool ok;
   Test_Data td;
   Eina_File *f;
   char path[PATH_MAX];

   elm_init(1, NULL);
   win = elm_win_add(NULL, "image", ELM_WIN_BASIC);

   td.success = 1; // skip "error" case
   td.image_id = 1;

   sprintf(path, pathfmt, td.image_id);
   f = eina_file_open(path, 0);
   ck_assert(f);

   image = elm_image_add(win);
   elm_image_async_open_set(image, 1);
   evas_object_smart_callback_add(image, "load,error", _async_error_cb, &td);
   evas_object_smart_callback_add(image, "load,open", _async_opened_cb, &td);
   evas_object_smart_callback_add(image, "load,ready", _async_ready_cb, &td);
   evas_object_show(image);
   ok = elm_image_mmap_set(image, f, NULL);
   ck_assert(ok);

   ecore_timer_add(10.0, _timeout_cb, &td);

   elm_run();
   ck_assert(td.success == 3);

   eina_file_close(f);

   elm_shutdown();
}
END_TEST

START_TEST (efl_ui_image_icon)
{
   Evas_Object *win, *image;
   Eina_Bool ok;
   const char *icon_name;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "image", ELM_WIN_BASIC);

   image = efl_add(EFL_UI_IMAGE_CLASS, win);
   evas_object_show(image);

   ok = efl_ui_image_icon_set(image, "folder");
   ck_assert(ok);
   icon_name = efl_ui_image_icon_get(image);
   ck_assert_str_eq(icon_name, "folder");

   ok = efl_ui_image_icon_set(image, "None");
   ck_assert(ok == 0);
   icon_name = efl_ui_image_icon_get(image);
   ck_assert(icon_name == NULL);

   elm_shutdown();
}
END_TEST

void elm_test_image(TCase *tc)
{
    tcase_add_test(tc, elm_atspi_role_get);
    tcase_add_test(tc, elm_image_async_path);
    tcase_add_test(tc, elm_image_async_mmap);
    tcase_add_test(tc, efl_ui_image_icon);
}
