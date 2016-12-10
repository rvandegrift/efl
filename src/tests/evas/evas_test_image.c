#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Evas.h>

#include "evas_suite.h"
#include "evas_tests_helpers.h"

#define TESTS_IMG_DIR TESTS_SRC_DIR"/images"

static const char *exts[] = {
  "png"
#ifdef BUILD_LOADER_TGA
  ,"tga"
#endif
#ifdef BUILD_LOADER_WBMP
  ,"wbmp"
#endif
#ifdef BUILD_LOADER_XPM
  ,"xpm"
#endif
#ifdef BUILD_LOADER_BMP
  ,"bmp"
#endif
#ifdef BUILD_LOADER_GIF
  ,"gif"
#endif
#ifdef BUILD_LOADER_PSD
  ,"psd"
#endif
#ifdef BUILD_LOADER_WEBP
  ,"webp"
#endif
#ifdef BUILD_LOADER_JPEG
  ,"jpeg"
  ,"jpg"
#endif
#ifdef BUILD_LOADER_TGV
  ,"tgv"
#endif
};

START_TEST(evas_object_image_loader)
{
   Evas *e = _setup_evas();
   Evas_Object *o;
   Eina_Iterator *it;
   const Eina_File_Direct_Info *file;

   o = evas_object_image_add(e);

   it = eina_file_direct_ls(TESTS_IMG_DIR);
   EINA_ITERATOR_FOREACH(it, file)
     {
        Eina_Bool found = EINA_FALSE;
        unsigned int i;
        int w, h;

        for (i = 0; i < (sizeof (exts) / sizeof (exts[0])); i++)
          if (!strcasecmp(file->path + file->path_length - strlen(exts[i]),
                          exts[i]))
            {
               found = EINA_TRUE;
               break;
            }

        if (!found) continue;

        evas_object_image_file_set(o, file->path, NULL);
        fail_if(evas_object_image_load_error_get(o) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(o, &w, &h);
        fail_if(w == 0 || h == 0);
     }
   eina_iterator_free(it);

   evas_object_del(o);

   evas_free(e);
   evas_shutdown();
}
END_TEST

typedef struct _Orientation_Test_Res Orientation_Test_Res;
struct _Orientation_Test_Res {
   const char *img;
   const char *desc;
   Evas_Image_Orient orient;
   int (*compare_func)(const uint32_t *d1, const uint32_t *d2, int w2, int h2);
};

typedef struct _orient_Test {
   Evas_Image_Orient orient;
   const char *desc;
   int (*compare_func)(const uint32_t *d1, const uint32_t *d2, int w2, int h2);
} Orient_Test;

static int _compare_img(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   return memcmp(d1, d2, w2 * h2 * 4);
}

static int _compare_img_90(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (x = w2 - 1; x >= 0; x--)
     {
        for (y = 0; y < h2; y++)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_180(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (y = h2 - 1; y >= 0; y--)
     {
        for (x = w2 - 1; x >= 0; x--)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_270(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (x = 0; x < w2; x++)
     {
        for (y = h2 - 1; y >= 0; y--)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_flip_h(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (y = 0; y < h2; y++)
     {
        for (x = w2 - 1; x >= 0; x--)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_flip_v(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (y = h2 - 1; y >= 0; y--)
     {
        for (x = 0; x < w2; x++)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_transpose(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (x = 0; x < w2; x++)
     {
        for (y = 0; y < h2; y++)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

static int _compare_img_transverse(const uint32_t *d1, const uint32_t *d2, int w2, int h2)
{
   int x, y;
   int r;

   for (x = w2 - 1; x >= 0; x--)
     {
        for (y = h2 - 1; y >= 0; y--)
          {
             r = *d1 - *(d2 + x + y * w2);
             if (r != 0) return r;
             d1++;
          }
     }

   return 0;
}

START_TEST(evas_object_image_loader_orientation)
{
   Evas *e = _setup_evas();
   Evas_Object *orig, *rot;
   static const Orientation_Test_Res res[] = {
     { TESTS_IMG_DIR"/Light_exif.jpg", "Original", EVAS_IMAGE_ORIENT_NONE, _compare_img },
     { TESTS_IMG_DIR"/Light_exif_flip_h.jpg", "Flip horizontally", EVAS_IMAGE_FLIP_HORIZONTAL, _compare_img_flip_h },
     { TESTS_IMG_DIR"/Light_exif_180.jpg", "Rotate 180° CW", EVAS_IMAGE_ORIENT_180, _compare_img_180 },
     { TESTS_IMG_DIR"/Light_exif_flip_v.jpg", "Flip vertically", EVAS_IMAGE_FLIP_VERTICAL, _compare_img_flip_v },
     { TESTS_IMG_DIR"/Light_exif_transpose.jpg", "Transpose", EVAS_IMAGE_FLIP_TRANSPOSE, _compare_img_transpose },
     { TESTS_IMG_DIR"/Light_exif_90.jpg", "Rotate 90° CW", EVAS_IMAGE_ORIENT_90, _compare_img_90 },
     { TESTS_IMG_DIR"/Light_exif_transverse.jpg", "Transverse", EVAS_IMAGE_FLIP_TRANSVERSE, _compare_img_transverse },
     { TESTS_IMG_DIR"/Light_exif_270.jpg", "Rotate 90° CCW", EVAS_IMAGE_ORIENT_270, _compare_img_270 },
     { NULL, NULL, EVAS_IMAGE_ORIENT_NONE, NULL }
   };
   int w, h, r_w, r_h;
   const uint32_t *d, *r_d;
   int i;

   orig = evas_object_image_add(e);
   evas_object_image_file_set(orig, TESTS_IMG_DIR"/Light.jpg", NULL);
   fail_if(evas_object_image_load_error_get(orig) != EVAS_LOAD_ERROR_NONE);
   evas_object_image_size_get(orig, &w, &h);
   fail_if(w == 0 || h == 0);
   d = evas_object_image_data_get(orig, EINA_FALSE);

   rot = evas_object_image_add(e);
   evas_object_image_load_orientation_set(rot, EINA_TRUE);

   for (i = 0; res[i].img; i++)
     {
        evas_object_image_file_set(rot, res[i].img, NULL);
        fail_if(evas_object_image_load_error_get(rot) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(rot, &r_w, &r_h);
        fail_if(w * h != r_w * r_h);

        r_d = evas_object_image_data_get(rot, EINA_FALSE);

        fail_if(res[i].compare_func(d, r_d, r_w, r_h),
                "Image orientation test failed: exif orientation flag: %s\n", res[i].desc);
     }

   evas_object_del(orig);
   evas_object_del(rot);

   evas_free(e);
   evas_shutdown();
}
END_TEST

START_TEST(evas_object_image_orient)
{
   Evas *e = _setup_evas();
   Evas_Object *orig;
   Orient_Test res[] = {
       {EVAS_IMAGE_ORIENT_0, "Original", _compare_img},
       {EVAS_IMAGE_FLIP_HORIZONTAL, "Flip horizontally", _compare_img_flip_h},
       {EVAS_IMAGE_ORIENT_180, "Rotate 180° CW", _compare_img_180},
       {EVAS_IMAGE_FLIP_VERTICAL, "Flip vertically", _compare_img_flip_v},
       {EVAS_IMAGE_FLIP_TRANSPOSE, "Transpose", _compare_img_transpose},
       {EVAS_IMAGE_ORIENT_90, "Rotate 90° CW", _compare_img_90},
       {EVAS_IMAGE_FLIP_TRANSVERSE, "Transverse", _compare_img_transverse},
       {EVAS_IMAGE_ORIENT_270, "Rotate 90° CCW", _compare_img_270},
       {0, NULL, NULL}
   };
   int w, h, r_w, r_h;
   uint32_t *d, *r_d;
   int i;

   orig = evas_object_image_add(e);
   evas_object_image_file_set(orig, TESTS_IMG_DIR"/Light.jpg", NULL);
   fail_if(evas_object_image_load_error_get(orig) != EVAS_LOAD_ERROR_NONE);
   evas_object_image_size_get(orig, &w, &h);
   fail_if(w == 0 || h == 0);

   d = malloc(w * h * 4);
   fail_if(!d);
   r_d = evas_object_image_data_get(orig, EINA_FALSE);
   memcpy(d, r_d, w * h * 4);

   for (i = 0; res[i].desc; i++)
     {
        evas_object_image_orient_set(orig, res[i].orient);
        fail_if(evas_object_image_orient_get(orig) != res[i].orient);
        evas_object_image_size_get(orig, &r_w, &r_h);
        fail_if(w * h != r_w * r_h);

        r_d = evas_object_image_data_get(orig, EINA_FALSE);

        fail_if(res[i].compare_func(d, r_d, r_w, r_h),
                "Image orientation test failed: orient flag: %s\n", res[i].desc);
     }

   evas_object_del(orig);

   evas_free(e);
   evas_shutdown();
}
END_TEST

START_TEST(evas_object_image_tgv_loader_data)
{
   Evas *e = _setup_evas();
   Evas_Object *obj, *ref;
   Eina_Strbuf *str;

   const char *files[] = {
     "Light-50",
     "Pic1-10",
     "Pic1-100",
     "Pic1-50",
     "Pic4-10",
     "Pic4-100",
     "Pic4-50",
     "Train-10"
   };
   unsigned int i;

   obj = evas_object_image_add(e);
   ref = evas_object_image_add(e);
   str = eina_strbuf_new();

   for (i = 0; i < sizeof (files) / sizeof (files[0]); i++)
     {
        int w, h, r_w, r_h;
        const uint32_t *d, *r_d;

        eina_strbuf_append_printf(str, "%s/%s.tgv", TESTS_IMG_DIR, files[i]);
        evas_object_image_file_set(obj, eina_strbuf_string_get(str), NULL);
        fail_if(evas_object_image_load_error_get(obj) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(obj, &w, &h);
        d = evas_object_image_data_get(obj, EINA_FALSE);

        eina_strbuf_reset(str);

        eina_strbuf_append_printf(str, "%s/%s.png", TESTS_IMG_DIR, files[i]);
        evas_object_image_file_set(ref, eina_strbuf_string_get(str), NULL);
        fail_if(evas_object_image_load_error_get(ref) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(ref, &r_w, &r_h);
        r_d = evas_object_image_data_get(ref, EINA_FALSE);

        eina_strbuf_reset(str);

        fail_if(w != r_w || h != r_h);
        fail_if(memcmp(d, r_d, w * h * 4));
     }

   evas_object_del(obj);
   evas_object_del(ref);

   evas_free(e);
   evas_shutdown();
}
END_TEST

START_TEST(evas_object_image_all_loader_data)
{
   Evas *e = _setup_evas();
   Evas_Object *obj, *ref;
   Eina_Strbuf *str;
   unsigned int i;

   obj = evas_object_image_add(e);
   ref = evas_object_image_add(e);
   str = eina_strbuf_new();

   for (i = 0; i < sizeof (exts) / sizeof (exts[0]); i++)
     {
        struct stat st;
        int w, h, s, r_w, r_h, r_s;
        const uint32_t *d, *r_d;
        Evas_Colorspace c, r_c;

        eina_strbuf_reset(str);

        eina_strbuf_append_printf(str, "%s/Pic4-%s.png", TESTS_IMG_DIR, exts[i]);

        if (stat(eina_strbuf_string_get(str), &st) != 0) continue;

        evas_object_image_file_set(obj, eina_strbuf_string_get(str), NULL);
        fail_if(evas_object_image_load_error_get(obj) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(obj, &w, &h);
        s = evas_object_image_stride_get(obj);
        c = evas_object_image_colorspace_get(obj);
        d = evas_object_image_data_get(obj, EINA_FALSE);

        eina_strbuf_reset(str);

        eina_strbuf_append_printf(str, "%s/Pic4.%s", TESTS_IMG_DIR, exts[i]);
        evas_object_image_file_set(ref, eina_strbuf_string_get(str), NULL);
        fail_if(evas_object_image_load_error_get(ref) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(ref, &r_w, &r_h);
        r_s = evas_object_image_stride_get(ref);
        r_c = evas_object_image_colorspace_get(ref);
        r_d = evas_object_image_data_get(ref, EINA_FALSE);

        fail_if(w != r_w || h != r_h);
        fail_if(s != r_s);
        fail_if(c != r_c);
        fail_if(w*4 != s);
        if (strcmp(exts[i], "jpeg") == 0 || strcmp(exts[i], "jpg") == 0)
          {
             //jpeg norm allows a variation of 1 bit per component
             for (int j = 0; j < s * h; j++)
               {
                  fail_if(abs(((char*)d)[j] - ((char*)r_d)[j]) > 1);
               }
          }
        else
          {
             fail_if(memcmp(d, r_d, w * h * 4));
          }
     }

   evas_object_del(obj);
   evas_object_del(ref);

   eina_strbuf_free(str);

   evas_free(e);
   evas_shutdown();
}
END_TEST

const char *buggy[] = {
  "BMP301K"
};

START_TEST(evas_object_image_buggy)
{
   Evas *e = _setup_evas();
   Evas_Object *obj, *ref;
   Eina_Strbuf *str;
   unsigned int i, j;

   obj = evas_object_image_add(e);
   ref = evas_object_image_add(e);
   str = eina_strbuf_new();

   for (i = 0; i < sizeof (buggy) / sizeof (buggy[0]); i++)
     {
        for (j = 0; j < sizeof (exts) / sizeof (exts[0]); j++)
          {
             struct stat st;
             int w, h, r_w, r_h;
             const uint32_t *d, *r_d;

             eina_strbuf_reset(str);

             if (!strcmp(exts[j], "png")) continue ;

             eina_strbuf_append_printf(str, "%s/%s.%s", TESTS_IMG_DIR,
                                       buggy[i], exts[j]);

             if (stat(eina_strbuf_string_get(str), &st) != 0) continue;

             evas_object_image_file_set(obj, eina_strbuf_string_get(str), NULL);
             fail_if(evas_object_image_load_error_get(obj) != EVAS_LOAD_ERROR_NONE);
             evas_object_image_size_get(obj, &w, &h);
             d = evas_object_image_data_get(obj, EINA_FALSE);

             eina_strbuf_reset(str);

             eina_strbuf_append_printf(str, "%s/%s.png", TESTS_IMG_DIR,
                                       buggy[i]);
             evas_object_image_file_set(ref, eina_strbuf_string_get(str), NULL);
             fail_if(evas_object_image_load_error_get(ref) != EVAS_LOAD_ERROR_NONE);
             evas_object_image_size_get(ref, &r_w, &r_h);
             r_d = evas_object_image_data_get(ref, EINA_FALSE);

             fail_if(w != r_w || h != r_h);
             fail_if(memcmp(d, r_d, w * h * 4));
          }
     }

   evas_object_del(obj);
   evas_object_del(ref);

   eina_strbuf_free(str);

   evas_free(e);
   evas_shutdown();
}
END_TEST

static void check_rotate_region(Evas_Image_Orient orientation, int *r_x, int *r_y, int *r_w, int *r_h, int w, int h)
{
   int tmp;

   switch (orientation)
     {
      case EVAS_IMAGE_FLIP_HORIZONTAL:
         *r_x = w - *r_w;
         break;
      case EVAS_IMAGE_FLIP_VERTICAL:
         *r_y = h - *r_h;
         break;
      case EVAS_IMAGE_ORIENT_180:
        *r_x = w - *r_w;
        *r_y = h - *r_h;
        break;
      case EVAS_IMAGE_ORIENT_90:
        tmp = *r_x;
        *r_x = w - (*r_y + *r_h);
        *r_y = tmp;
        tmp = *r_w;
        *r_w = *r_h;
        *r_h = tmp;
        break;
      case EVAS_IMAGE_ORIENT_270:
        tmp = *r_y;
        *r_y = h - (*r_x + *r_w);
        *r_x = tmp;
        tmp = *r_w;
        *r_w = *r_h;
        *r_h = tmp;
        break;
      case EVAS_IMAGE_FLIP_TRANSPOSE:
        tmp = *r_x;
        *r_x = *r_y;
        *r_y = tmp;
        tmp = *r_w;
        *r_w = *r_h;
        *r_h = tmp;
        break;
      case EVAS_IMAGE_FLIP_TRANSVERSE:
        tmp = *r_x;
        *r_x = w - (*r_y + *r_h);
        *r_y = h - (tmp + *r_w);
        tmp = *r_w;
        *r_w = *r_h;
        *r_h = tmp;
        break;
      case EVAS_IMAGE_ORIENT_0:
         break;
     }
}


START_TEST(evas_object_image_partially_load_orientation)
{
   static const Orientation_Test_Res res[] = {
     { TESTS_IMG_DIR"/Light_exif.jpg", "Original", EVAS_IMAGE_ORIENT_NONE, _compare_img },
     { TESTS_IMG_DIR"/Light_exif_flip_h.jpg", "Flip horizontally", EVAS_IMAGE_FLIP_HORIZONTAL, _compare_img_flip_h },
     { TESTS_IMG_DIR"/Light_exif_180.jpg", "Rotate 180° CW", EVAS_IMAGE_ORIENT_180, _compare_img_180 },
     { TESTS_IMG_DIR"/Light_exif_flip_v.jpg", "Flip vertically", EVAS_IMAGE_FLIP_VERTICAL, _compare_img_flip_v },
     { TESTS_IMG_DIR"/Light_exif_transpose.jpg", "Transpose", EVAS_IMAGE_FLIP_TRANSPOSE, _compare_img_transpose },
     { TESTS_IMG_DIR"/Light_exif_90.jpg", "Rotate 90° CW", EVAS_IMAGE_ORIENT_90, _compare_img_90 },
     { TESTS_IMG_DIR"/Light_exif_transverse.jpg", "Transverse", EVAS_IMAGE_FLIP_TRANSVERSE, _compare_img_transverse },
     { TESTS_IMG_DIR"/Light_exif_270.jpg", "Rotate 90° CCW", EVAS_IMAGE_ORIENT_270, _compare_img_270 },
     { NULL, NULL, EVAS_IMAGE_ORIENT_NONE, NULL }
   };

   Evas *e = _setup_evas();
   Evas_Object *orig, *rot;
   int x, y, w, h, r_w, r_h;
   int region_x, region_y, region_w, region_h;
   const uint32_t *d, *r_d;
   int i;

   orig = evas_object_image_add(e);
   evas_object_image_file_set(orig, TESTS_IMG_DIR"/Light.jpg", NULL);
   fail_if(evas_object_image_load_error_get(orig) != EVAS_LOAD_ERROR_NONE);
   evas_object_image_size_get(orig, &w, &h);
   x = 0; y = 0; w = w / 2; h = h / 2;;
   evas_object_image_load_region_set(orig, x, y, w, h);
   evas_object_image_size_get(orig, &w, &h);
   d = evas_object_image_data_get(orig, EINA_FALSE);
   for (i = 0; res[i].img; i++)
     {
        region_x = x;
        region_y = y;
        region_w = w;
        region_h = h;
        rot = evas_object_image_add(e);
        evas_object_image_load_orientation_set(rot, EINA_TRUE);
        evas_object_image_file_set(rot, res[i].img, NULL);
        fail_if(evas_object_image_load_error_get(rot) != EVAS_LOAD_ERROR_NONE);
        evas_object_image_size_get(rot, &r_w, &r_h);
        check_rotate_region(res[i].orient, &region_x, &region_y, &region_w, &region_h, r_w, r_h);
        evas_object_image_load_region_set(rot, region_x, region_y, region_w, region_h);
        evas_object_image_size_get(rot, &r_w, &r_h);
        fail_if(w * h != r_w * r_h);
        r_d = evas_object_image_data_get(rot, EINA_FALSE);
        fail_if(res[i].compare_func(d, r_d, r_w, r_h),
                "Image orientation partially load test failed: exif orientation flag: %s\n", res[i].desc);
        evas_object_del(rot);
     }

   evas_object_del(orig);

   evas_free(e);
   evas_shutdown();
}
END_TEST

START_TEST(evas_object_image_defaults)
{
   Evas *e = _setup_evas();
   Evas_Object *o;
   int x, y, w, h;

   /* test legacy defaults */
   o = evas_object_image_add(e);
   fail_if(evas_object_image_filled_get(o));
   evas_object_image_fill_get(o, &x, &y, &w, &h);
   fail_if(x || y || w || h);
   eo_del(o);

   o = evas_object_image_filled_add(e);
   fail_if(!evas_object_image_filled_get(o));
   eo_del(o);

   /* test eo defaults */
   o = eo_add(EFL_CANVAS_IMAGE_CLASS, e);
   fail_if(!efl_gfx_fill_auto_get(o));
   eo_del(o);

   evas_free(e);
   evas_shutdown();
}
END_TEST

// FIXME: belongs to another file
START_TEST(evas_object_image_map_unmap)
{
   Evas *e = _setup_evas();
   Evas_Object *o, *o2;
   void *data;
   int len, stride;
   int w, h, rx, ry, rw, rh;
   Efl_Gfx_Colorspace cs;
   Eina_Tmpstr *tmp;
   int fd;
   uint32_t *data32;
   uint8_t *data8;
   Eina_Bool all_white = 1, all_transparent = 1;

   const char *imgpath = TESTS_IMG_DIR "/Pic4.png";

   o = eo_add(EFL_CANVAS_IMAGE_CLASS, e);
   efl_file_set(o, imgpath, NULL);
   efl_gfx_view_size_get(o, &w, &h);
   cs = efl_gfx_buffer_colorspace_get(o);

   rx = (w / 4) & ~3;
   ry = (h / 4) & ~3;
   rw = (w / 2) & ~3;
   rh = (h / 2) & ~3;

   // same cspace, full image
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ, 0, 0, w, h, cs, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   efl_gfx_buffer_unmap(o, data, len);

   // same cspace, partial image
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ, rx, ry, rw, rh, cs, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   efl_gfx_buffer_unmap(o, data, len);

   // argb cspace, full image
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ, 0, 0, w, h, EFL_GFX_COLORSPACE_ARGB8888, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   data32 = data;
   for (int k = 0; (k < len) && (all_white || all_transparent); k++)
     {
        if (data32[k])
          all_transparent = 0;
        if (data32[k] != 0xFFFFFFFF)
          all_white = 0;
     }
   fail_if(all_white || all_transparent);
   efl_gfx_buffer_unmap(o, data, len);

   // argb cspace, partial image
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ, rx, ry, rw, rh, EFL_GFX_COLORSPACE_ARGB8888, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   efl_gfx_buffer_unmap(o, data, len);

   // argb cspace, partial image, write
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_WRITE, rx, ry, rw, rh, EFL_GFX_COLORSPACE_ARGB8888, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   data32 = data;
   for (int y = 0; y < rh; y += 2)
     for (int x = 0; x < rw; x++)
       {
          data32[y*stride/4 + x] = 0xFF00FF00;
          data32[(y+1)*stride/4 + x] = 0xFFFF0000;
       }
   efl_gfx_buffer_unmap(o, data, len);

   // argb cspace, partial image, write
   data = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ| EFL_GFX_BUFFER_ACCESS_MODE_WRITE,
                             rx, ry, rw, rh / 2, EFL_GFX_COLORSPACE_GRY8, &stride);
   fail_if(!data);
   fail_if(!len);
   fail_if(!stride);
   data8 = data;
   for (int y = 0; y < rh / 4; y++)
     for (int x = 0; x < rw; x++)
       data8[y*stride + x] = x & 0xFF;
   efl_gfx_buffer_unmap(o, data, len);

   // save file, verify its pixels
   fd = eina_file_mkstemp("/tmp/evas-test.XXXXXX.png", &tmp);
   close(fd);
   if (efl_file_save(o, tmp, NULL, NULL))
     {
        int w2, h2, stride2, len2;
        uint32_t *data2, *orig;
        int x, y;

        o2 = eo_add(EFL_CANVAS_IMAGE_CLASS, e);
        efl_file_set(o2, tmp, NULL);
        efl_gfx_view_size_get(o, &w2, &h2);

        // unlink now to not leave any crap after failing the test
        unlink(tmp);

        fail_if(w2 != w);
        fail_if(h2 != h);

        orig = efl_gfx_buffer_map(o, &len, EFL_GFX_BUFFER_ACCESS_MODE_READ, 0, 0, w, h, EFL_GFX_COLORSPACE_ARGB8888, &stride);
        fail_if(!orig);
        fail_if(!len);
        fail_if(!stride);

        data2 = efl_gfx_buffer_map(o2, &len2, EFL_GFX_BUFFER_ACCESS_MODE_READ, 0, 0, w2, h2, EFL_GFX_COLORSPACE_ARGB8888, &stride2);
        fail_if(!data2);
        fail_if(len2 != len);
        fail_if(stride2 != stride);

        // first quarter: same image
        for (y = 0; y < h / 4; y++)
          for (x = 0; x < w; x++)
            fail_if(orig[y*stride/4 + x] != data2[y*stride2/4+x], "pixels differ [1]");

        // middle zone top: grey gradient
        for (y = ry; y < (ry + rh / 4); y++)
          for (x = rx; x < rx + rw; x++)
            {
               uint32_t c = (x - rx) & 0xFF;
               c = 0xFF000000 | (c << 16) | (c << 8) | c;
               fail_if(data2[y*stride/4 + x] != c, "pixels differ [2]");
            }

        // middle zone: grey image
        for (y = (ry + rh / 4 + 1); y < (ry + rh / 2); y++)
          for (x = rx; x < rx + rw; x++)
            {
               uint32_t c = data2[y*stride/4 + x] & 0xFF;
               c = 0xFF000000 | (c << 16) | (c << 8) | c;
               fail_if(data2[y*stride/4 + x] != c, "pixels differ [2bis]");
            }

        // next lines: green & red
        y = ry + rh / 2;
        for (x = rx; x < rx + rw; x++)
          {
             fail_if(data2[y*stride/4 + x] != 0xFF00FF00, "pixels differ [3]");
             fail_if(data2[(y+1)*stride/4 + x] != 0xFFFF0000, "pixels differ [4]");
          }

        efl_gfx_buffer_unmap(o, orig, len);
        efl_gfx_buffer_unmap(o2, data2, len2);
     }
   else unlink(tmp);
   eina_tmpstr_del(tmp);

   // TODO: test copy-on-write
   // TODO: test more color conversions

   evas_free(e);
   evas_shutdown();
}
END_TEST

void evas_test_image_object(TCase *tc)
{
   tcase_add_test(tc, evas_object_image_defaults);
   tcase_add_test(tc, evas_object_image_loader);
   tcase_add_test(tc, evas_object_image_loader_orientation);
   tcase_add_test(tc, evas_object_image_orient);
#if BUILD_LOADER_TGV && BUILD_LOADER_PNG
   tcase_add_test(tc, evas_object_image_tgv_loader_data);
#endif
#if BUILD_LOADER_PNG
   tcase_add_test(tc, evas_object_image_all_loader_data);
   tcase_add_test(tc, evas_object_image_buggy);
   tcase_add_test(tc, evas_object_image_map_unmap);
#endif
   tcase_add_test(tc, evas_object_image_partially_load_orientation);
}
