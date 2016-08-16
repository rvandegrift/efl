#include "evas_image_private.h"

#define MY_CLASS EFL_CANVAS_IMAGE_INTERNAL_CLASS
#define MY_CLASS_NAME "Evas_Image"

/* private magic number for image objects */
static const char o_type[] = "image";

const char *o_image_type = o_type;

/* private methods for image objects */
static Evas_Coord evas_object_image_figure_x_fill(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Coord start, Evas_Coord size, Evas_Coord *size_ret);
static Evas_Coord evas_object_image_figure_y_fill(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Coord start, Evas_Coord size, Evas_Coord *size_ret);

static void evas_object_image_init(Evas_Object *eo_obj);
static void evas_object_image_render(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj,
				     void *type_private_data,
				     void *output, void *context, void *surface,
				     int x, int y, Eina_Bool do_async);
static void _evas_image_render(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                               void *output, void *context, void *surface,
                               int x, int y, int l, int t, int r, int b, Eina_Bool do_async);
static void evas_object_image_free(Evas_Object *eo_obj,
				   Evas_Object_Protected_Data *obj);
static void evas_object_image_render_pre(Evas_Object *eo_obj,
                                         Evas_Object_Protected_Data *obj,
					 void *type_private_data);
static void evas_object_image_render_post(Evas_Object *eo_obj,
					  Evas_Object_Protected_Data *obj,
					  void *type_private_data);

static unsigned int evas_object_image_id_get(Evas_Object *eo_obj);
static unsigned int evas_object_image_visual_id_get(Evas_Object *eo_obj);
static void *evas_object_image_engine_data_get(Evas_Object *eo_obj);

static int evas_object_image_is_opaque(Evas_Object *eo_obj,
				       Evas_Object_Protected_Data *obj,
				       void *type_private_data);
static int evas_object_image_was_opaque(Evas_Object *eo_obj,
					Evas_Object_Protected_Data *obj,
					void *type_private_data);
static int evas_object_image_is_inside(Evas_Object *eo_obj,
				       Evas_Object_Protected_Data *obj,
				       void *type_private_data,
				       Evas_Coord x, Evas_Coord y);
static int evas_object_image_has_opaque_rect(Evas_Object *eo_obj,
					     Evas_Object_Protected_Data *obj,
					     void *type_private_data);
static int evas_object_image_get_opaque_rect(Evas_Object *eo_obj,
					     Evas_Object_Protected_Data *obj,
					     void *type_private_data,
					     Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
static int evas_object_image_can_map(Evas_Object *eo_obj);

static void evas_object_image_filled_resize_listener(void *data, Evas *eo_e, Evas_Object *eo_obj, void *einfo);

static const Evas_Object_Func object_func =
{
   /* methods (compulsory) */
   NULL,
     evas_object_image_render,
     evas_object_image_render_pre,
     evas_object_image_render_post,
     evas_object_image_id_get,
     evas_object_image_visual_id_get,
     evas_object_image_engine_data_get,
     /* these are optional. NULL = nothing */
     NULL,
     NULL,
     NULL,
     NULL,
     evas_object_image_is_opaque,
     evas_object_image_was_opaque,
     evas_object_image_is_inside,
     NULL,
     NULL,
     NULL,
     evas_object_image_has_opaque_rect,
     evas_object_image_get_opaque_rect,
     evas_object_image_can_map,
     NULL
};

static const Evas_Object_Image_Load_Opts default_load_opts = {
  0, 0.0, 0, 0, { 0, 0, 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0 }, 0
};

static const Evas_Object_Image_Pixels default_pixels = {
  NULL, { NULL, NULL }, { 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, ~0x0
};

static const Evas_Object_Image_State default_state = {
  { 0, 0, 0, 0 }, // fill
  { 0, 0, 0 }, // image
  { 1.0, 0, 0, 0, 0, 1 }, // border
  NULL, NULL, NULL,  //source, defmap, scene
  { NULL }, //u
  NULL, //key
  0, //frame
  EVAS_COLORSPACE_ARGB8888,
  EVAS_IMAGE_ORIENT_NONE,

  EINA_TRUE, // smooth
  EINA_FALSE, // has_alpha
  EINA_FALSE, // opaque_valid
  EINA_FALSE, // opaque
  EINA_FALSE // mmapped_source
};

Eina_Cow *evas_object_image_load_opts_cow = NULL;
Eina_Cow *evas_object_image_pixels_cow = NULL;
Eina_Cow *evas_object_image_state_cow = NULL;

void
_evas_image_cleanup(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *o)
{
   /* Eina_Cow doesn't know if the resulting memory has changed, better check
      before we change it */
   if (o->cur->opaque_valid)
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->opaque_valid = 0;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }

   if ((o->preloading) && (o->engine_data))
     {
        o->preloading = EINA_FALSE;
        ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
     }
   if (o->cur->source) _evas_image_proxy_unset(eo_obj, obj, o);
   if (o->cur->scene) _evas_image_3d_unset(eo_obj, obj, o);
}

static Eina_Bool
_init_cow(void)
{
   if (!evas_object_image_load_opts_cow ||
       !evas_object_image_pixels_cow ||
       !evas_object_image_state_cow)
     {
        evas_object_image_load_opts_cow = eina_cow_add("Evas_Object_Image load opts",
                                                       sizeof (Evas_Object_Image_Load_Opts),
                                                       8,
                                                       &default_load_opts,
                                                       EINA_TRUE);
        evas_object_image_pixels_cow = eina_cow_add("Evas_Object_Image pixels",
                                                    sizeof (Evas_Object_Image_Pixels),
                                                    8,
                                                    &default_pixels,
                                                    EINA_TRUE);
        evas_object_image_state_cow = eina_cow_add("Evas_Object_Image states",
                                                   sizeof (Evas_Object_Image_State),
                                                   8,
                                                   &default_state,
                                                   EINA_TRUE);
     }
   if (!evas_object_image_load_opts_cow ||
       !evas_object_image_pixels_cow ||
       !evas_object_image_state_cow)
     {
        ERR("Failed to init cow.");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EOLIAN static Eo *
_efl_canvas_image_internal_eo_base_constructor(Eo *eo_obj, Evas_Image_Data *o)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Colorspace cspace;

   eo_obj = eo_constructor(eo_super(eo_obj, MY_CLASS));

   evas_object_image_init(eo_obj);

   if (!_init_cow())
     return NULL;

   o->load_opts = eina_cow_alloc(evas_object_image_load_opts_cow);
   o->pixels = eina_cow_alloc(evas_object_image_pixels_cow);
   o->cur = eina_cow_alloc(evas_object_image_state_cow);
   o->prev = eina_cow_alloc(evas_object_image_state_cow);
   o->proxy_src_clip = EINA_TRUE;

   cspace = ENFN->image_colorspace_get(ENDT, o->engine_data);
   if (cspace != o->cur->cspace)
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          state_write->cspace = cspace;
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }

   return eo_obj;
}

EOLIAN static Eo *
_efl_canvas_image_internal_eo_base_finalize(Eo *eo_obj, Evas_Image_Data *o)
{
   if (!o->filled_set)
     efl_gfx_fill_auto_set(eo_obj, EINA_TRUE);
   return eo_finalize(eo_super(eo_obj, MY_CLASS));
}

void
_evas_image_init_set(const Eina_File *f, const char *file, const char *key,
                Eo *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *o,
                Evas_Image_Load_Opts *lo)
{
   if (o->cur->source) _evas_image_proxy_unset(eo_obj, obj, o);
   if (o->cur->scene) _evas_image_3d_unset(eo_obj, obj, o);

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     {
        if (f)
          {
             if (!state_write->mmaped_source)
               eina_stringshare_del(state_write->u.file);
             else if (state_write->u.f)
               eina_file_close(state_write->u.f);
             state_write->u.f = eina_file_dup(f);
          }
        else
          {
             if (!state_write->mmaped_source)
               eina_stringshare_replace(&state_write->u.file, file);
             else
               {
                  if (state_write->u.f) eina_file_close(state_write->u.f);
                  state_write->u.file = eina_stringshare_add(file);
               }
          }
        state_write->mmaped_source = !!f;
        eina_stringshare_replace(&state_write->key, key);

        state_write->opaque_valid = 0;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   if (o->prev->u.file != NULL || o->prev->key != NULL)
     {
        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->u.file = NULL;
             state_write->key = NULL;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);
     }

   if (o->engine_data)
     {
        if (o->preloading)
          {
             o->preloading = EINA_FALSE;
             ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
          }
        ENFN->image_free(ENDT, o->engine_data);
     }
   if (o->file_obj)
     {
        eo_del(o->file_obj);
        o->file_obj = NULL;
     }
   o->load_error = EVAS_LOAD_ERROR_NONE;
   lo->scale_down_by = o->load_opts->scale_down_by;
   lo->dpi = o->load_opts->dpi;
   lo->w = o->load_opts->w;
   lo->h = o->load_opts->h;
   lo->region.x = o->load_opts->region.x;
   lo->region.y = o->load_opts->region.y;
   lo->region.w = o->load_opts->region.w;
   lo->region.h = o->load_opts->region.h;
   lo->scale_load.src_x = o->load_opts->scale_load.src_x;
   lo->scale_load.src_y = o->load_opts->scale_load.src_y;
   lo->scale_load.src_w = o->load_opts->scale_load.src_w;
   lo->scale_load.src_h = o->load_opts->scale_load.src_h;
   lo->scale_load.dst_w = o->load_opts->scale_load.dst_w;
   lo->scale_load.dst_h = o->load_opts->scale_load.dst_h;
   lo->scale_load.smooth = o->load_opts->scale_load.smooth;
   lo->scale_load.scale_hint = o->load_opts->scale_load.scale_hint;
   lo->orientation = o->load_opts->orientation;
   lo->degree = 0;
}

void
_evas_image_done_set(Eo *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *o)
{
   Eina_Bool resize_call = EINA_FALSE;

   if (o->engine_data)
     {
        int w, h;
        int stride;

        ENFN->image_size_get(ENDT, o->engine_data, &w, &h);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENDT, o->engine_data, &stride);
        else
          stride = w * 4;

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->has_alpha = ENFN->image_alpha_get(ENDT, o->engine_data);
             state_write->cspace = ENFN->image_colorspace_get(ENDT, o->engine_data);

             if ((o->cur->image.w != w) || (o->cur->image.h != h))
               resize_call = EINA_TRUE;

             state_write->image.w = w;
             state_write->image.h = h;
             state_write->image.stride = stride;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }
   else
     {
        if (o->load_error == EVAS_LOAD_ERROR_NONE)
          o->load_error = EVAS_LOAD_ERROR_GENERIC;

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->has_alpha = EINA_TRUE;
             state_write->cspace = EVAS_COLORSPACE_ARGB8888;

             if ((state_write->image.w != 0) || (state_write->image.h != 0))
               resize_call = EINA_TRUE;

             state_write->image.w = 0;
             state_write->image.h = 0;
             state_write->image.stride = 0;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }
   o->written = EINA_FALSE;
   o->changed = EINA_TRUE;
   if (resize_call) evas_object_inform_call_image_resize(eo_obj);
   evas_object_change(eo_obj, obj);
}

void
_evas_image_orientation_set(Eo *eo_obj, Evas_Image_Data *o, Evas_Image_Orient orient)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   int iw, ih;

   if (o->cur->orient == orient) return;

   if ((o->preloading) && (o->engine_data))
     {
        o->preloading = EINA_FALSE;
        ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
     }

   if (o->engine_data)
     {
        int stride = 0;

        o->engine_data = ENFN->image_orient_set(ENDT, o->engine_data, orient);
        if(o->engine_data)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
              state_write->orient = orient;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

             if (ENFN->image_stride_get)
               ENFN->image_stride_get(ENDT, o->engine_data, &stride);
             else
               stride = o->cur->image.w * 4;
             if (o->cur->image.stride != stride)
               {
                  EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
                   state_write->image.stride = stride;
                  EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
               }
             o->written = EINA_TRUE;
          }
        ENFN->image_size_get(ENDT, o->engine_data, &iw, &ih);
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->image.w = iw;
             state_write->image.h = ih;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }
   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

static Evas_Image_Orient
_get_image_orient_from_orient_flip(Efl_Orient orient, Efl_Flip flip)
{
   switch (orient)
     {
      case EFL_ORIENT_0:
         if (flip == EFL_FLIP_HORIZONTAL)
           return EVAS_IMAGE_FLIP_HORIZONTAL;
         else if (flip == EFL_FLIP_VERTICAL)
           return EVAS_IMAGE_FLIP_VERTICAL;
         else
           return EVAS_IMAGE_ORIENT_0;
      case EFL_ORIENT_90:
         if (flip == EFL_FLIP_HORIZONTAL)
           return EVAS_IMAGE_FLIP_TRANSPOSE;
         else if (flip == EFL_FLIP_VERTICAL)
           return EVAS_IMAGE_FLIP_TRANSVERSE;
         else
           return EVAS_IMAGE_ORIENT_90;
      case EFL_ORIENT_180:
         if (flip == EFL_FLIP_HORIZONTAL)
           return EVAS_IMAGE_FLIP_VERTICAL;
         else if (flip == EFL_FLIP_VERTICAL)
           return EVAS_IMAGE_FLIP_HORIZONTAL;
         else
           return EVAS_IMAGE_ORIENT_180;
      case EFL_ORIENT_270:
         if (flip == EFL_FLIP_HORIZONTAL)
           return EVAS_IMAGE_FLIP_TRANSVERSE;
         else if (flip == EFL_FLIP_VERTICAL)
           return EVAS_IMAGE_FLIP_TRANSPOSE;
         else
           return EVAS_IMAGE_ORIENT_270;
      default:
         return EVAS_IMAGE_ORIENT_NONE;
     }
}

EOLIAN static void
_efl_canvas_image_internal_efl_orientation_orientation_set(Eo *obj, Evas_Image_Data *o, Efl_Orient dir)
{
   Evas_Image_Orient orient;

   o->orient_value = dir;
   orient = _get_image_orient_from_orient_flip(dir, o->flip_value);

   _evas_image_orientation_set(obj, o, orient);
}

EOLIAN static Efl_Orient
_efl_canvas_image_internal_efl_orientation_orientation_get(Eo *obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->orient_value;
}

EOLIAN static void
_efl_canvas_image_internal_efl_flipable_flip_set(Eo *obj, Evas_Image_Data *o, Efl_Flip flip)
{
   Evas_Image_Orient orient;

   o->flip_value = flip;
   orient = _get_image_orient_from_orient_flip(o->orient_value, flip);

   _evas_image_orientation_set(obj, o, orient);
}

EOLIAN static Efl_Flip
_efl_canvas_image_internal_efl_flipable_flip_get(Eo *obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->flip_value;
}

EOLIAN static void
_efl_canvas_image_internal_eo_base_dbg_info_get(Eo *eo_obj, Evas_Image_Data *o, Eo_Dbg_Info *root)
{
   eo_dbg_info_get(eo_super(eo_obj, MY_CLASS), root);
   Eo_Dbg_Info *group = EO_DBG_INFO_LIST_APPEND(root, MY_CLASS_NAME);

   const char *file, *key;
   if (o->cur->mmaped_source)
     file = eina_file_filename_get(o->cur->u.f);
   else
     file = o->cur->u.file;
   key = o->cur->key;

   EO_DBG_INFO_APPEND(group, "Image File", EINA_VALUE_TYPE_STRING, file);
   EO_DBG_INFO_APPEND(group, "Key", EINA_VALUE_TYPE_STRING, key);
   EO_DBG_INFO_APPEND(group, "Source", EINA_VALUE_TYPE_UINT64,
                          (uint64_t) (uintptr_t) evas_object_image_source_get(eo_obj));

   if (efl_image_load_error_get(eo_obj) != EFL_IMAGE_LOAD_ERROR_NONE)
     {
        Evas_Load_Error error = EVAS_LOAD_ERROR_GENERIC;
        error = (Evas_Load_Error) _evas_image_load_error_get(eo_obj);
        EO_DBG_INFO_APPEND(group, "Load Error", EINA_VALUE_TYPE_STRING,
                           evas_load_error_str(error));
     }
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_border_set(Eo *eo_obj, Evas_Image_Data *o, int l, int r, int t, int b)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (l < 0) l = 0;
   if (r < 0) r = 0;
   if (t < 0) t = 0;
   if (b < 0) b = 0;
   if ((o->cur->border.l == l) &&
       (o->cur->border.r == r) &&
       (o->cur->border.t == t) &&
       (o->cur->border.b == b)) return;

   evas_object_async_block(obj);
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     {
        state_write->border.l = l;
        state_write->border.r = r;
        state_write->border.t = t;
        state_write->border.b = b;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_border_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o, int *l, int *r, int *t, int *b)
{
   if (l) *l = o->cur->border.l;
   if (r) *r = o->cur->border.r;
   if (t) *t = o->cur->border.t;
   if (b) *b = o->cur->border.b;
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_border_center_fill_set(Eo *eo_obj, Evas_Image_Data *o, Efl_Gfx_Border_Fill_Mode _fill)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Border_Fill_Mode fill = (Evas_Border_Fill_Mode) _fill;

   if (fill == o->cur->border.fill) return;
   evas_object_async_block(obj);
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->border.fill = fill;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static Efl_Gfx_Border_Fill_Mode
_efl_canvas_image_internal_efl_image_border_center_fill_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return (Efl_Gfx_Border_Fill_Mode) o->cur->border.fill;
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_fill_fill_auto_set(Eo *eo_obj, Evas_Image_Data* o, Eina_Bool setting)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   setting = !!setting;
   o->filled_set = 1;
   if (o->filled == setting) return;

   evas_object_async_block(obj);
   o->filled = setting;
   if (!o->filled)
     evas_object_event_callback_del(eo_obj, EVAS_CALLBACK_RESIZE,
                                    evas_object_image_filled_resize_listener);
   else
     {
        Evas_Coord w, h;

        efl_gfx_geometry_get(eo_obj, NULL, NULL, &w, &h);
        _evas_image_fill_set(eo_obj, o, 0, 0, w, h);

        evas_object_event_callback_add(eo_obj, EVAS_CALLBACK_RESIZE,
                                       evas_object_image_filled_resize_listener,
                                       NULL);
     }
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_gfx_fill_fill_auto_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->filled;
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_border_scale_set(Eo *eo_obj, Evas_Image_Data *o, double scale)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (scale == o->cur->border.scale) return;
   evas_object_async_block(obj);
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->border.scale = scale;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static double
_efl_canvas_image_internal_efl_image_border_scale_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->cur->border.scale;
}

void
_evas_image_fill_set(Eo *eo_obj, Evas_Image_Data *o, int x, int y, int w, int h)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (w == 0) return;
   if (h == 0) return;
   if (w < 0) w = -w;
   if (h < 0) h = -h;

   if ((o->cur->fill.x == x) &&
       (o->cur->fill.y == y) &&
       (o->cur->fill.w == w) &&
       (o->cur->fill.h == h)) return;

   evas_object_async_block(obj);
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     {
        state_write->fill.x = x;
        state_write->fill.y = y;
        state_write->fill.w = w;
        state_write->fill.h = h;
        state_write->opaque_valid = 0;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_fill_fill_set(Eo *eo_obj, Evas_Image_Data *o,
                                  int x, int y, int w, int h)
{
   // Should (0,0,0,0) reset the filled flag to true?
   o->filled = EINA_FALSE;
   o->filled_set = EINA_TRUE;
   _evas_image_fill_set(eo_obj, o, x, y, w, h);
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_fill_fill_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o,
                                  int *x, int *y, int *w, int *h)
{
   if (x) *x = o->cur->fill.x;
   if (y) *y = o->cur->fill.y;
   if (w) *w = o->cur->fill.w;
   if (h) *h = o->cur->fill.h;
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_view_view_size_get(Eo *eo_obj, Evas_Image_Data *o, int *w, int *h)
{
   int uvw, uvh;
   Evas_Object_Protected_Data *source = NULL;
   Evas_Object_Protected_Data *obj;

   obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   if (o->cur->source)
     source = eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);

   if (o->cur->scene)
     {
        uvw = obj->data_3d->w;
        uvh = obj->data_3d->h;
     }
   else if (!o->cur->source)
     {
        uvw = o->cur->image.w;
        uvh = o->cur->image.h;
     }
   else if (source->proxy->surface && !source->proxy->redraw)
     {
        uvw = source->proxy->w;
        uvh = source->proxy->h;
     }
   else if (source->type == o_type &&
            ((Evas_Image_Data *)eo_data_scope_get(o->cur->source, MY_CLASS))->engine_data)
     {
        uvw = source->cur->geometry.w;
        uvh = source->cur->geometry.h;
     }
   else
     {
        uvw = source->proxy->w;
        uvh = source->proxy->h;
     }

   if (w) *w = uvw;
   if (h) *h = uvh;
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_buffer_buffer_update_add(Eo *eo_obj, Evas_Image_Data *o, int x, int y, int w, int h)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Eina_Rectangle *r;
   int cnt;

   RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, o->cur->image.w, o->cur->image.h);
   if ((w <= 0)  || (h <= 0)) return;
   if (!o->written) return;
   evas_object_async_block(obj);
   cnt = eina_list_count(o->pixels->pixel_updates);
   if (cnt == 1)
     { // detect single blob case already there to do a nop
        if ((r = o->pixels->pixel_updates->data))
          { // already a single full rect there.
             if ((r->x == 0) && (r->y == 0) && (r->w == o->cur->image.w) && (r->h == o->cur->image.h))
               return;
          }
     }
   if ((cnt >= 512) ||
       (((x == 0) && (y == 0) && (w == o->cur->image.w) && (h == o->cur->image.h))))
     { // too many update rects - just make a single blob update
        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          {
             EINA_LIST_FREE(pixi_write->pixel_updates, r) eina_rectangle_free(r);
             NEW_RECT(r, 0, 0, o->cur->image.w, o->cur->image.h);
             if (r) pixi_write->pixel_updates = eina_list_append(pixi_write->pixel_updates, r);
          }
        EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }
   else
     {
        NEW_RECT(r, x, y, w, h);
        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          if (r) pixi_write->pixel_updates = eina_list_append(pixi_write->pixel_updates, r);
        EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_buffer_alpha_set(Eo *eo_obj, Evas_Image_Data *o, Eina_Bool has_alpha)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if ((o->preloading) && (o->engine_data))
     {
        o->preloading = EINA_FALSE;
        ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
     }

   has_alpha = !!has_alpha;
   if (has_alpha == o->cur->has_alpha)
     return;

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->has_alpha = has_alpha;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   if (o->engine_data)
     {
        int stride = 0;

        o->engine_data = ENFN->image_alpha_set(ENDT, o->engine_data, o->cur->has_alpha);
        if (ENFN->image_scale_hint_set)
          ENFN->image_scale_hint_set(ENDT, o->engine_data, o->scale_hint);
        if (ENFN->image_content_hint_set)
          ENFN->image_content_hint_set (ENDT, o->engine_data, o->content_hint);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENDT, o->engine_data, &stride);
        else
          stride = o->cur->image.w * 4;

        if (o->cur->image.stride != stride)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
               state_write->image.stride = stride;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
          }
        o->written = EINA_TRUE;
     }
   efl_gfx_buffer_update_add(eo_obj, 0, 0, o->cur->image.w, o->cur->image.h);
   EVAS_OBJECT_WRITE_IMAGE_FREE_FILE_AND_KEY(o);
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_gfx_buffer_alpha_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->cur->has_alpha;
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_smooth_scale_set(Eo *eo_obj, Evas_Image_Data *o, Eina_Bool smooth_scale)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if (((smooth_scale) && (o->cur->smooth_scale)) ||
       ((!smooth_scale) && (!o->cur->smooth_scale)))
     return;
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     state_write->smooth_scale = smooth_scale;
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

   o->changed = EINA_TRUE;
   evas_object_change(eo_obj, obj);
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_image_smooth_scale_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->cur->smooth_scale;
}

EOLIAN static double
_efl_canvas_image_internal_efl_image_ratio_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   if (!o->cur->image.h) return 1.0;
   return (double) o->cur->image.w / (double) o->cur->image.h;
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_file_save(const Eo *eo_obj, Evas_Image_Data *o, const char *file, const char *key, const char *flags)
{
   DATA32 *data = NULL;
   int quality = 80, compress = 9, ok = 0;
   char *encoding = NULL;
   Image_Entry *ie;
   Eina_Bool putback = EINA_FALSE, tofree = EINA_FALSE, no_convert = EINA_FALSE;
   Evas_Colorspace cspace = EVAS_COLORSPACE_ARGB8888;
   int want_cspace = EVAS_COLORSPACE_ARGB8888;
   int imagew, imageh;
   void *pixels;

   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Object_Protected_Data *source = (o->cur->source ? eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS) : NULL);

   evas_object_async_block(obj);

   if (o->cur->scene)
     {
        _evas_image_3d_render(obj->layer->evas->evas, (Eo *) eo_obj, obj, o, o->cur->scene);
        pixels = obj->data_3d->surface;
        imagew = obj->data_3d->w;
        imageh = obj->data_3d->h;
     }
   else if (!o->cur->source)
     {
        // pixels = evas_process_dirty_pixels(eo_obj, obj, o, output, surface, o->engine_data);
        pixels = o->engine_data;
        imagew = o->cur->image.w;
        imageh = o->cur->image.h;
        putback = EINA_TRUE;
     }
   else if (source->proxy->surface && !source->proxy->redraw)
     {
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
     }
   else if (source->type == o_type &&
            ((Evas_Image_Data *)eo_data_scope_get(o->cur->source, MY_CLASS))->engine_data)
     {
        Evas_Image_Data *oi;
        oi = eo_data_scope_get(o->cur->source, MY_CLASS);
        pixels = oi->engine_data;
        imagew = oi->cur->image.w;
        imageh = oi->cur->image.h;
     }
   else
     {
        o->proxyrendering = EINA_TRUE;
        evas_render_proxy_subrender(obj->layer->evas->evas, o->cur->source,
                                    (Eo *) eo_obj, obj, EINA_FALSE);
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
        o->proxyrendering = EINA_FALSE;
     }

   if (flags)
     {
        const char *ext = NULL;
        char *p, *pp;
        char *tflags;

        tflags = alloca(strlen(flags) + 1);
        strcpy(tflags, flags);
        p = tflags;
        while (p)
          {
             pp = strchr(p, ' ');
             if (pp) *pp = 0;
             sscanf(p, "quality=%i", &quality);
             sscanf(p, "compress=%i", &compress);
             sscanf(p, "encoding=%ms", &encoding);
             if (pp) p = pp + 1;
             else break;
          }

        if (file) ext = strrchr(file, '.');
        if (encoding && ext && !strcasecmp(ext, ".tgv"))
          {
             if (!strcmp(encoding, "auto"))
               want_cspace = -1;
             else if (!strcmp(encoding, "etc1"))
               want_cspace = EVAS_COLORSPACE_ETC1;
             else if (!strcmp(encoding, "etc2"))
               {
                  if (!ENFN->image_alpha_get(ENDT, pixels))
                    want_cspace = EVAS_COLORSPACE_RGB8_ETC2;
                  else
                    want_cspace = EVAS_COLORSPACE_RGBA8_ETC2_EAC;
               }
             else if (!strcmp(encoding, "etc1+alpha"))
               want_cspace = EVAS_COLORSPACE_ETC1_ALPHA;
          }
        else
          {
             free(encoding);
             encoding = NULL;
          }
     }

   if (!ENFN->image_data_direct)
     pixels = ENFN->image_data_get(ENDT, pixels, 0, &data, &o->load_error, &tofree);
   else
     {
        if (ENFN->image_data_direct(ENDT, pixels, &cspace))
          {
             if ((want_cspace != (int) cspace) && (want_cspace != -1))
               cspace = EVAS_COLORSPACE_ARGB8888;
          }
        else
          {
             cspace = ENFN->image_file_colorspace_get(ENDT, pixels);
             if ((want_cspace != (int) cspace) && (want_cspace != -1))
               cspace = EVAS_COLORSPACE_ARGB8888;
             else
               {
                  ENFN->image_colorspace_set(ENDT, pixels, cspace);
                  no_convert = EINA_TRUE;
               }
          }
        pixels = ENFN->image_data_get(ENDT, pixels, 0, &data, &o->load_error, &tofree);
     }

   if (!pixels)
     {
        WRN("Could not get image pixels.");
        return EINA_FALSE;
     }

   switch (cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
        break;
      case EVAS_COLORSPACE_ETC1:
      case EVAS_COLORSPACE_ETC1_ALPHA:
      case EVAS_COLORSPACE_RGB8_ETC2:
      case EVAS_COLORSPACE_RGBA8_ETC2_EAC:
        break;
      default:
        DBG("Need to convert colorspace before saving");
        cspace = EVAS_COLORSPACE_ARGB8888;
        break;
     }

   ie = evas_cache_image_data(evas_common_image_cache_get(),
                              imagew, imageh, data, o->cur->has_alpha, cspace);
   if (ie)
     {
        RGBA_Image *im = (RGBA_Image *) ie;
        DATA32 *old_data = NULL;

        // FIXME: Something is fishy here... what about the previous pointer?
        if ((o->cur->cspace == cspace) || no_convert)
          im->image.data = data;
        else
          {
             old_data = im->image.data;
             im->image.data = _evas_image_data_convert_internal(o, data, EVAS_COLORSPACE_ARGB8888);
          }
        if (im->image.data)
          {
             ok = evas_common_save_image_to_file(im, file, key, quality, compress, encoding);

             if (old_data)
               {
                  free(im->image.data);
                  im->image.data = old_data;
               }
          }
        evas_cache_image_drop(ie);
     }

   if (tofree)
     ENFN->image_free(ENDT, pixels);
   else if (putback)
     o->engine_data = ENFN->image_data_put(ENDT, pixels, data);

   free(encoding);
   return ok;
}

EOLIAN static Efl_Gfx_Colorspace
_efl_canvas_image_internal_efl_gfx_buffer_colorspace_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return (Efl_Gfx_Colorspace) o->cur->cspace;
}

static void
_on_image_native_surface_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *einfo EINA_UNUSED)
{
   evas_object_image_native_surface_set(obj, NULL);
}

Eina_Bool
_evas_image_native_surface_set(Eo *eo_obj, Evas_Native_Surface *surf)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_async_block(obj);
   evas_object_event_callback_del_full
     (eo_obj, EVAS_CALLBACK_DEL, _on_image_native_surface_del, NULL);
   if (surf) // We need to unset native surf on del to remove shared hash refs
     evas_object_event_callback_add
     (eo_obj, EVAS_CALLBACK_DEL, _on_image_native_surface_del, NULL);

   evas_render_rendering_wait(obj->layer->evas);
   _evas_image_cleanup(eo_obj, obj, o);
   if (!ENFN->image_native_set) return EINA_FALSE;
   if ((surf) &&
       ((surf->version < 2) ||
        (surf->version > EVAS_NATIVE_SURFACE_VERSION))) return EINA_FALSE;
   o->engine_data = ENFN->image_native_set(ENDT, o->engine_data, surf);
   return (o->engine_data != NULL);
}

Evas_Native_Surface *
_evas_image_native_surface_get(const Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Evas_Native_Surface *surf = NULL;

   if (ENFN->image_native_get)
     surf = ENFN->image_native_get(ENDT, o->engine_data);

   return surf;
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_scale_hint_set(Eo *eo_obj, Evas_Image_Data *o, Efl_Image_Scale_Hint hint)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_async_block(obj);
   if (o->scale_hint == hint) return;
   o->scale_hint = hint;
   if (o->engine_data)
     {
        int stride = 0;

        if (ENFN->image_scale_hint_set)
          ENFN->image_scale_hint_set(ENDT, o->engine_data, o->scale_hint);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENDT, o->engine_data, &stride);
        else
          stride = o->cur->image.w * 4;

        if (o->cur->image.stride != stride)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
               state_write->image.stride = stride;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
          }
     }
}

EOLIAN static Efl_Image_Scale_Hint
_efl_canvas_image_internal_efl_image_scale_hint_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->scale_hint;
}

EOLIAN static void
_efl_canvas_image_internal_efl_image_content_hint_set(Eo *eo_obj, Evas_Image_Data *o, Efl_Image_Content_Hint hint)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   evas_object_async_block(obj);
   if (o->content_hint == hint) return;
   o->content_hint = hint;
   if (o->engine_data)
     {
        int stride = 0;

        if (ENFN->image_content_hint_set)
          ENFN->image_content_hint_set(ENDT, o->engine_data, o->content_hint);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENDT, o->engine_data, &stride);
        else
          stride = o->cur->image.w * 4;

        if (o->cur->image.stride != stride)
          {
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
               state_write->image.stride = stride;
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
          }
     }
}

EOLIAN static Efl_Image_Content_Hint
_efl_canvas_image_internal_efl_image_content_hint_get(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o)
{
   return o->content_hint;
}

EOLIAN void
_evas_canvas_image_cache_flush(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   evas_canvas_async_block(e);
   evas_render_rendering_wait(e);
   if (e->engine.data.output)
     e->engine.func->image_cache_flush(e->engine.data.output);
}

EOLIAN void
_evas_canvas_image_cache_reload(Eo *eo_e, Evas_Public_Data *e)
{
   Evas_Layer *layer;

   evas_canvas_async_block(e);
   evas_image_cache_flush(eo_e);
   EINA_INLIST_FOREACH(e->layers, layer)
     {
        Evas_Object_Protected_Data *obj;

        layer->walking_objects++;
        EINA_INLIST_FOREACH(layer->objects, obj)
          {
             if (eo_isa(obj->object, MY_CLASS))
               {
                  _evas_image_unload(obj->object, obj, 1);
                  evas_object_inform_call_image_unloaded(obj->object);
               }
          }
        layer->walking_objects--;
        _evas_layer_flush_removes(layer);
     }
   evas_image_cache_flush(eo_e);
   EINA_INLIST_FOREACH(e->layers, layer)
     {
        Evas_Object_Protected_Data *obj;

        layer->walking_objects++;
        EINA_INLIST_FOREACH(layer->objects, obj)
          {
             if (eo_isa(obj->object, MY_CLASS))
               {
                  Evas_Image_Data *o = eo_data_scope_get(obj->object, MY_CLASS);
                  _evas_image_load(obj->object, obj, o);
                  o->changed = EINA_TRUE;
                  evas_object_change(obj->object, obj);
               }
          }
        layer->walking_objects--;
        _evas_layer_flush_removes(layer);
     }
   evas_image_cache_flush(eo_e);
}

EOLIAN void
_evas_canvas_image_cache_set(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, int size)
{
   if (size < 0) size = 0;
   evas_canvas_async_block(e);
   evas_render_rendering_wait(e);
   if (e->engine.data.output)
     e->engine.func->image_cache_set(e->engine.data.output, size);
}

EOLIAN int
_evas_canvas_image_cache_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e)
{
   if (e->engine.data.output)
     return e->engine.func->image_cache_get(e->engine.data.output);
   return -1;
}

EOLIAN Eina_Bool
_evas_canvas_image_max_size_get(Eo *eo_e EINA_UNUSED, Evas_Public_Data *e, int *maxw, int *maxh)
{
   int w = 0, h = 0;

   if (maxw) *maxw = 0xffff;
   if (maxh) *maxh = 0xffff;
   if (!e->engine.func->image_max_size_get) return EINA_FALSE;
   e->engine.func->image_max_size_get(e->engine.data.output, &w, &h);
   if (maxw) *maxw = w;
   if (maxh) *maxh = h;
   return EINA_TRUE;
}

void
_evas_image_unload(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Eina_Bool dirty)
{
   Evas_Image_Data *o;
   Eina_Bool resize_call = EINA_FALSE;

   o = eo_data_scope_get(eo_obj, MY_CLASS);
   if ((!o->cur->u.file) ||
       (o->pixels_checked_out > 0)) return;

   evas_object_async_block(obj);
   if (dirty)
     {
        if (o->engine_data)
           o->engine_data = ENFN->image_dirty_region(ENDT, o->engine_data,
                                                     0, 0,
                                                     o->cur->image.w, o->cur->image.h);
     }
   if (o->engine_data)
     {
        if (o->preloading)
          {
             o->preloading = EINA_FALSE;
             ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
          }
        ENFN->image_free(ENDT, o->engine_data);
     }
   o->engine_data = NULL;
   o->load_error = EVAS_LOAD_ERROR_NONE;

   EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
     {
        state_write->has_alpha = EINA_TRUE;
        state_write->cspace = EVAS_COLORSPACE_ARGB8888;
        if ((state_write->image.w != 0) || (state_write->image.h != 0)) resize_call = EINA_TRUE;
        state_write->image.w = 0;
        state_write->image.h = 0;
        state_write->image.stride = 0;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
   if (resize_call) evas_object_inform_call_image_resize(eo_obj);
}

void
_evas_image_load(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *o)
{
   Evas_Image_Load_Opts lo;

   if (o->engine_data) return;

   lo.scale_down_by = o->load_opts->scale_down_by;
   lo.dpi = o->load_opts->dpi;
   lo.w = o->load_opts->w;
   lo.h = o->load_opts->h;
   lo.region.x = o->load_opts->region.x;
   lo.region.y = o->load_opts->region.y;
   lo.region.w = o->load_opts->region.w;
   lo.region.h = o->load_opts->region.h;
   lo.scale_load.src_x = o->load_opts->scale_load.src_x;
   lo.scale_load.src_y = o->load_opts->scale_load.src_y;
   lo.scale_load.src_w = o->load_opts->scale_load.src_w;
   lo.scale_load.src_h = o->load_opts->scale_load.src_h;
   lo.scale_load.dst_w = o->load_opts->scale_load.dst_w;
   lo.scale_load.dst_h = o->load_opts->scale_load.dst_h;
   lo.scale_load.smooth = o->load_opts->scale_load.smooth;
   lo.scale_load.scale_hint = o->load_opts->scale_load.scale_hint;
   lo.orientation = o->load_opts->orientation;
   lo.degree = 0;
   if (o->cur->mmaped_source)
     o->engine_data = ENFN->image_mmap(ENDT, o->cur->u.f, o->cur->key, &o->load_error, &lo);
   else
     {
        const char *file2 = o->cur->u.file;

        if (file2)
          {
             o->file_obj = efl_vpath_manager_fetch(EFL_VPATH_MANAGER_CLASS, file2);
             efl_vpath_file_do(o->file_obj);
             // XXX:FIXME: allow this to be async
             efl_vpath_file_wait(o->file_obj);
             file2 = efl_vpath_file_result_get(o->file_obj);
          }
        o->engine_data = ENFN->image_load(ENDT, file2, o->cur->key, &o->load_error, &lo);
     }

   if (o->engine_data)
     {
        int w, h;
        int stride = 0;
        Eina_Bool resize_call = EINA_FALSE;

        ENFN->image_size_get(ENDT, o->engine_data, &w, &h);
        if (ENFN->image_stride_get)
          ENFN->image_stride_get(ENDT, o->engine_data, &stride);
        else
          stride = w * 4;

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->has_alpha = ENFN->image_alpha_get(ENDT, o->engine_data);
             state_write->cspace = ENFN->image_colorspace_get(ENDT, o->engine_data);
             if ((state_write->image.w != w) || (state_write->image.h != h))
               resize_call = EINA_TRUE;
             state_write->image.w = w;
             state_write->image.h = h;
             state_write->image.stride = stride;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
        if (resize_call) evas_object_inform_call_image_resize(eo_obj);
     }
   else
     {
        o->load_error = EVAS_LOAD_ERROR_GENERIC;
     }
}

static Evas_Coord
evas_object_image_figure_x_fill(Evas_Object *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, Evas_Coord start, Evas_Coord size, Evas_Coord *size_ret)
{
   Evas_Coord w;

   w = ((size * obj->layer->evas->output.w) /
        (Evas_Coord)obj->layer->evas->viewport.w);
   if (size <= 0) size = 1;
   if (start > 0)
     {
        while (start - size > 0) start -= size;
     }
   else if (start < 0)
     {
        while (start < 0) start += size;
     }
   start = ((start * obj->layer->evas->output.w) /
            (Evas_Coord)obj->layer->evas->viewport.w);
   *size_ret = w;
   return start;
}

static Evas_Coord
evas_object_image_figure_y_fill(Evas_Object *eo_obj EINA_UNUSED, Evas_Object_Protected_Data *obj, Evas_Coord start, Evas_Coord size, Evas_Coord *size_ret)
{
   Evas_Coord h;

   h = ((size * obj->layer->evas->output.h) /
        (Evas_Coord)obj->layer->evas->viewport.h);
   if (size <= 0) size = 1;
   if (start > 0)
     {
        while (start - size > 0) start -= size;
     }
   else if (start < 0)
     {
        while (start < 0) start += size;
     }
   start = ((start * obj->layer->evas->output.h) /
            (Evas_Coord)obj->layer->evas->viewport.h);
   *size_ret = h;
   return start;
}

static void
evas_object_image_init(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   /* set up methods (compulsory) */
   obj->func = &object_func;
   obj->private_data = eo_data_ref(eo_obj, MY_CLASS);
   obj->type = o_type;
}

EOLIAN static void
_efl_canvas_image_internal_eo_base_destructor(Eo *eo_obj, Evas_Image_Data *o EINA_UNUSED)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   if (obj->legacy)
     evas_object_image_video_surface_set(eo_obj, NULL);
   evas_object_image_free(eo_obj, obj);
   eo_destructor(eo_super(eo_obj, MY_CLASS));
}

void
_evas_object_image_free(Evas_Object *obj)
{
   Evas_Image_Data *o;

   EINA_SAFETY_ON_FALSE_RETURN(eo_isa(obj, MY_CLASS));

   o = eo_data_scope_get(obj, MY_CLASS);

   // eina_cow_free reset the pointer to the default read only state
   eina_cow_free(evas_object_image_load_opts_cow, (const Eina_Cow_Data**) &o->load_opts);
   eina_cow_free(evas_object_image_pixels_cow, (const Eina_Cow_Data**) &o->pixels);
   eina_cow_free(evas_object_image_state_cow, (const Eina_Cow_Data**) &o->cur);
   eina_cow_free(evas_object_image_state_cow, (const Eina_Cow_Data**) &o->prev);
}

static void
evas_object_image_free(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Eina_Rectangle *r;

   /* free obj */
   if (!o->cur->mmaped_source)
     {
        if (o->cur->u.file) eina_stringshare_del(o->cur->u.file);
     }
   else
     {
        if (o->cur->u.f) eina_file_close(o->cur->u.f);
     }
   if (o->cur->key) eina_stringshare_del(o->cur->key);
   if (o->cur->source) _evas_image_proxy_unset(eo_obj, obj, o);
   if (o->cur->scene) _evas_image_3d_unset(eo_obj, obj, o);
   if (obj->layer && obj->layer->evas)
     {
       if (o->engine_data)
	 {
	   if (o->preloading)
	     {
	       o->preloading = EINA_FALSE;
               ENFN->image_data_preload_cancel(ENDT, o->engine_data, eo_obj);
	     }
           ENFN->image_free(ENDT, o->engine_data);
	 }
       if (o->video_surface)
	 {
	   o->video_surface = EINA_FALSE;
	   obj->layer->evas->video_objects = eina_list_remove(obj->layer->evas->video_objects, eo_obj);
	 }
     }
   o->engine_data = NULL;
   if (o->file_obj)
     {
        eo_del(o->file_obj);
        o->file_obj = NULL;
     }
   if (o->pixels->pixel_updates)
     {
       EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
	 {
	   EINA_LIST_FREE(pixi_write->pixel_updates, r)
	     eina_rectangle_free(r);
	 }
       EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }
}

static void
_draw_image(Evas_Object_Protected_Data *obj,
            void *data, void *context, void *surface, void *image,
            int src_x, int src_y, int src_w, int src_h, int dst_x,
            int dst_y, int dst_w, int dst_h, int smooth,
            Eina_Bool do_async)
{
   Eina_Bool async_unref;

   async_unref = ENFN->image_draw(data, context, surface,
                                  image, src_x, src_y,
                                  src_w, src_h, dst_x,
                                  dst_y, dst_w, dst_h,
                                  smooth, do_async);
   if (do_async && async_unref)
     {
#ifdef EVAS_CSERVE2
        if (evas_cserve2_use_get())
          evas_cache2_image_ref((Image_Entry *)image);
        else
#endif
          evas_cache_image_ref((Image_Entry *)image);

        evas_unref_queue_image_put(obj->layer->evas, image);
     }
}

void
evas_draw_image_map_async_check(Evas_Object_Protected_Data *obj,
                                void *data, void *context, void *surface,
                                void *image, RGBA_Map *m, int smooth, int level,
                                Eina_Bool do_async)
{
   Eina_Bool async_unref;
   obj->layer->evas->engine.func->context_anti_alias_set(data, context,
                                                         obj->cur->anti_alias);
   async_unref = ENFN->image_map_draw(data, context,
                                      surface, image, m,
                                      smooth, level,
                                      do_async);
   if (do_async && async_unref)
     {
#ifdef EVAS_CSERVE2
        if (evas_cserve2_use_get())
          evas_cache2_image_ref((Image_Entry *)image);
        else
#endif
          evas_cache_image_ref((Image_Entry *)image);

        evas_unref_queue_image_put(obj->layer->evas, image);
     }
}

static void *
evas_process_dirty_pixels(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, Evas_Image_Data *o,
                          void *output, void *surface, void *pixels)
{
   Eina_Bool direct_override = EINA_FALSE, direct_force_off = EINA_FALSE;

   if (o->dirty_pixels)
     {
        if (o->pixels->func.get_pixels)
          {
             Evas_Coord x, y, w, h;

             if (ENFN->image_native_get)
               {
                  Evas_Native_Surface *ns;

                  ns = ENFN->image_native_get(ENDT, o->engine_data);
                  if (ns)
                    {
                       Eina_Bool direct_renderable = EINA_FALSE;

                       // Check if we can do direct rendering...
                       if (ENFN->gl_direct_override_get)
                         ENFN->gl_direct_override_get(output, &direct_override, &direct_force_off);
                       if (ENFN->gl_surface_direct_renderable_get)
                         direct_renderable = ENFN->gl_surface_direct_renderable_get(output, ns, &direct_override, surface);

                       if ( ((direct_override) ||
                             ((direct_renderable) &&
                              (obj->cur->geometry.w == o->cur->image.w) &&
                              (obj->cur->geometry.h == o->cur->image.h) &&
                              (obj->cur->color.r == 255) &&
                              (obj->cur->color.g == 255) &&
                              (obj->cur->color.b == 255) &&
                              (obj->cur->color.a == 255) &&
                              (obj->cur->cache.clip.r == 255) &&
                              (obj->cur->cache.clip.g == 255) &&
                              (obj->cur->cache.clip.b == 255) &&
                              (obj->cur->cache.clip.a == 255) &&
                              (!obj->map->cur.map))
                             ) && (!direct_force_off) )
                         {
                            if (ENFN->gl_get_pixels_set)
                              ENFN->gl_get_pixels_set(output, o->pixels->func.get_pixels, o->pixels->func.get_pixels_data, eo_obj);
                            if (ENFN->gl_image_direct_set)
                              ENFN->gl_image_direct_set(output, o->engine_data, EINA_TRUE);
                            o->direct_render = EINA_TRUE;
                         }
                       else
                         o->direct_render = EINA_FALSE;
                    }

                  if ( (ns) &&
                       (ns->type == EVAS_NATIVE_SURFACE_X11))
                    {
                       if (ENFN->context_flush)
                         ENFN->context_flush(output);
                    }
               }

             x = obj->cur->geometry.x;
             y = obj->cur->geometry.y;
             w = obj->cur->geometry.w;
             h = obj->cur->geometry.h;

             if (!o->direct_render)
               {
                  if (ENFN->gl_get_pixels_pre)
                    ENFN->gl_get_pixels_pre(output);
                  o->pixels->func.get_pixels(o->pixels->func.get_pixels_data, eo_obj);
                  if (ENFN->gl_get_pixels_post)
                    ENFN->gl_get_pixels_post(output);
               }

             if (!(obj->cur->geometry.x == x &&
                   obj->cur->geometry.y == y &&
                   obj->cur->geometry.w == w &&
                   obj->cur->geometry.h == h))
               CRI("Evas_Image_Data geometry did change during pixels get callback !");

             o->engine_data = ENFN->image_dirty_region
               (ENDT, o->engine_data,
                0, 0, o->cur->image.w, o->cur->image.h);
	     if (o->engine_data != pixels)
	       pixels = o->engine_data;
          }
        o->dirty_pixels = EINA_FALSE;
     }
   else
     {
        // Check if the it's not dirty but it has direct rendering
        if (o->direct_render && ENFN->image_native_get)
          {
             Evas_Native_Surface *ns;
             ns = ENFN->image_native_get(output, o->engine_data);
             if (ENFN->gl_direct_override_get)
               ENFN->gl_direct_override_get(output, &direct_override, &direct_force_off);
             if (ENFN->gl_surface_direct_renderable_get)
               ENFN->gl_surface_direct_renderable_get(output, ns, &direct_override, surface);

             if (direct_override && !direct_force_off)
               {
                  // always use direct rendering
                  if (ENFN->gl_get_pixels_set)
                    ENFN->gl_get_pixels_set(output, o->pixels->func.get_pixels, o->pixels->func.get_pixels_data, eo_obj);
                  if (ENFN->gl_image_direct_set)
                    ENFN->gl_image_direct_set(output, o->engine_data, EINA_TRUE);
               }
             else
               {
                  // Auto-fallback to FBO rendering (for perf & power consumption)
                  if (ENFN->gl_get_pixels_pre)
                    ENFN->gl_get_pixels_pre(output);
                  o->pixels->func.get_pixels(o->pixels->func.get_pixels_data, obj->object);
                  if (ENFN->gl_get_pixels_post)
                    ENFN->gl_get_pixels_post(output);
                  o->direct_render = EINA_FALSE;
               }
          }
     }

   return pixels;
}

EOLIAN static void
_efl_canvas_image_internal_efl_canvas_filter_internal_filter_dirty(Eo *eo_obj, Evas_Image_Data *o)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);

   o->changed = 1;
   evas_object_change(eo_obj, obj);
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_canvas_filter_internal_filter_input_alpha(Eo *eo_obj EINA_UNUSED, Evas_Image_Data *o EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static void
_efl_canvas_image_internal_efl_gfx_filter_filter_program_set(Eo *obj, Evas_Image_Data *pd,
                                                             const char *code, const char *name)
{
   pd->has_filter = (code != NULL);
   efl_gfx_filter_program_set(eo_super(obj, MY_CLASS), code, name);
}

EOLIAN static Eina_Bool
_efl_canvas_image_internal_efl_canvas_filter_internal_filter_input_render(Eo *eo_obj, Evas_Image_Data *o,
                                                           void *_filter, void *context,
                                                           int l, int r EINA_UNUSED, int t, int b EINA_UNUSED,
                                                           Eina_Bool do_async)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Filter_Context *filter = _filter;
   void *surface, *output;
   Eina_Bool input_stolen;
   int W, H;

   W = obj->cur->geometry.w;
   H = obj->cur->geometry.h;
   output = ENDT;

   if (ENFN->gl_surface_read_pixels)
     {
        surface = ENFN->image_map_surface_new(output, W, H, EINA_TRUE);
        input_stolen = EINA_FALSE;
     }
   else
     {
        surface = evas_filter_buffer_backing_steal(filter, EVAS_FILTER_BUFFER_INPUT_ID);
        input_stolen = EINA_TRUE;
     }
   if (!o->filled)
     {
        l = 0;
        t = 0;
        r = 0;
        b = 0;
     }

   if (!surface)
     {
        ERR("Failed to allocate surface for filter input!");
        return EINA_FALSE;
     }

   ENFN->context_color_set(output, context, 0, 0, 0, 0);
   ENFN->context_render_op_set(output, context, EVAS_RENDER_COPY);
   ENFN->rectangle_draw(output, context, surface, 0, 0, W, H, EINA_FALSE);
   ENFN->context_color_set(output, context, 255, 255, 255, 255);
   ENFN->context_render_op_set(output, context, EVAS_RENDER_BLEND);

   _evas_image_render(eo_obj, obj, output, context, surface,
                      l - obj->cur->geometry.x, t - obj->cur->geometry.y,
                      l, t, r, b, do_async);

   if (!input_stolen)
     {
        evas_filter_image_draw(filter, context, EVAS_FILTER_BUFFER_INPUT_ID, surface, do_async);
        ENFN->image_free(output, surface);
     }
   else
     evas_filter_buffer_backing_release(filter, surface);

   return EINA_TRUE;
}

static void
evas_object_image_render(Evas_Object *eo_obj, Evas_Object_Protected_Data *obj, void *type_private_data,
			 void *output, void *context, void *surface, int x, int y, Eina_Bool do_async)
{
   Evas_Image_Data *o = type_private_data;

   if ((o->cur->fill.w < 1) || (o->cur->fill.h < 1))
     return; /* no error message, already printed in pre_render */

   /* Proxy sanity */
   if (o->proxyrendering)
     {
        _evas_image_proxy_error(eo_obj, context, output, surface, x, y, EINA_FALSE);
        return;
     }

   /* Mask sanity */
   if (obj->mask->is_mask && (surface != obj->mask->surface))
     {
        ERR("Drawing a mask to another surface? Something's wrong...");
        return;
     }

   /* We are displaying the overlay */
   if (o->video_visible)
     {
        /* Create a transparent rectangle */
        ENFN->context_color_set(output, context, 0, 0, 0, 0);
        ENFN->context_multiplier_unset(output, context);
        ENFN->context_render_op_set(output, context, EVAS_RENDER_COPY);
        ENFN->rectangle_draw(output, context, surface,
                             obj->cur->geometry.x + x, obj->cur->geometry.y + y,
                             obj->cur->geometry.w, obj->cur->geometry.h,
                             do_async);

        return;
     }

   ENFN->context_color_set(output, context, 255, 255, 255, 255);

   if ((obj->cur->cache.clip.r == 255) &&
       (obj->cur->cache.clip.g == 255) &&
       (obj->cur->cache.clip.b == 255) &&
       (obj->cur->cache.clip.a == 255))
     {
        ENFN->context_multiplier_unset(output, context);
     }
   else
     ENFN->context_multiplier_set(output, context,
                                  obj->cur->cache.clip.r,
                                  obj->cur->cache.clip.g,
                                  obj->cur->cache.clip.b,
                                  obj->cur->cache.clip.a);

   ENFN->context_render_op_set(output, context, obj->cur->render_op);

   // Clear out the pixel get stuff..
   if (ENFN->gl_get_pixels_set)
     ENFN->gl_get_pixels_set(output, NULL, NULL, NULL);
   if (ENFN->gl_image_direct_set)
     ENFN->gl_image_direct_set(output, o->engine_data, EINA_FALSE);

   /* Render filter */
   if (o->has_filter)
     {
        if (evas_filter_object_render(eo_obj, obj, output, context, surface, x, y, do_async, EINA_FALSE))
          return;
     }

   _evas_image_render(eo_obj, obj, output, context, surface, x, y, 0, 0, 0, 0, do_async);
}

static void
_evas_image_render(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                   void *output, void *context, void *surface, int x, int y,
                   int l, int t, int r, int b, Eina_Bool do_async)
{
   Evas_Image_Data *o = obj->private_data, *oi = NULL;
   int imagew, imageh, uvw, uvh, cw, ch;
   void *pixels;

   Evas_Object_Protected_Data *source =
      (o->cur->source ?
       eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS):
       NULL);
   if (source && (source->type == o_type))
     oi = eo_data_scope_get(o->cur->source, MY_CLASS);

   if (o->cur->scene)
     {
        _evas_image_3d_render(obj->layer->evas->evas, eo_obj, obj, o, o->cur->scene);
        pixels = obj->data_3d->surface;
        imagew = obj->data_3d->w;
        imageh = obj->data_3d->h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (obj->cur->snapshot)
     {
        pixels = o->engine_data;
        imagew = o->cur->image.w;
        imageh = o->cur->image.h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (!o->cur->source || !source)
     {
        pixels = evas_process_dirty_pixels(eo_obj, obj, o, output, surface, o->engine_data);
        /* pixels = o->engine_data; */
        imagew = o->cur->image.w;
        imageh = o->cur->image.h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (source->proxy->surface && !source->proxy->redraw)
     {
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (oi && oi->engine_data)
     {
        pixels = oi->engine_data;
        if (oi->has_filter)
          {
             void *output_buffer = NULL;
             output_buffer = evas_filter_output_buffer_get(source->object);
             if (output_buffer)
               pixels = output_buffer;
          }
        imagew = oi->cur->image.w;
        imageh = oi->cur->image.h;
        uvw = source->cur->geometry.w;
        uvh = source->cur->geometry.h;
        /* check source_clip since we skip proxy_subrender here */
        if (o->proxy_src_clip && source->cur->clipper)
          {
             ENFN->context_clip_clip(ENDT, context,
                                     source->cur->clipper->cur->cache.clip.x + x,
                                     source->cur->clipper->cur->cache.clip.y + y,
                                     source->cur->clipper->cur->cache.clip.w,
                                     source->cur->clipper->cur->cache.clip.h);
          }
     }
   else
     {
        o->proxyrendering = EINA_TRUE;
        evas_render_proxy_subrender(obj->layer->evas->evas, o->cur->source,
                                    eo_obj, obj, EINA_FALSE);
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
        uvw = imagew;
        uvh = imageh;
        o->proxyrendering = EINA_FALSE;
     }

   if (ENFN->context_clip_get(ENDT, context, NULL, NULL, &cw, &ch) && (!cw || !ch))
     return;

   if (pixels)
     {
        Evas_Coord idw, idh, idx, idy;
        int ix, iy, iw, ih;

        if ((obj->map->cur.map) && (obj->map->cur.map->count > 3) && (obj->map->cur.usemap))
          {
             evas_object_map_update(eo_obj, x, y, imagew, imageh, uvw, uvh);

             evas_draw_image_map_async_check(
                 obj, output, context, surface, pixels, obj->map->spans,
                 o->cur->smooth_scale | obj->map->cur.map->smooth, 0, do_async);
          }
        else
          {
             int offx, offy;

             ENFN->image_scale_hint_set(output, pixels, o->scale_hint);
             idx = evas_object_image_figure_x_fill(eo_obj, obj, o->cur->fill.x, o->cur->fill.w, &idw);
             idy = evas_object_image_figure_y_fill(eo_obj, obj, o->cur->fill.y, o->cur->fill.h, &idh);
             if (idw < 1) idw = 1;
             if (idh < 1) idh = 1;
             if (idx > 0) idx -= idw;
             if (idy > 0) idy -= idh;

             offx = obj->cur->geometry.x + x;
             offy = obj->cur->geometry.y + y;

             while ((int)idx < obj->cur->geometry.w)
               {
                  Evas_Coord ydy;
                  int dobreak_w = 0;

                  ydy = idy;
                  ix = idx;
                  if ((o->cur->fill.w == obj->cur->geometry.w) &&
                      (o->cur->fill.x == 0))
                    {
                       dobreak_w = 1;
                       iw = obj->cur->geometry.w;
                    }
                  else
                    iw = ((int)(idx + idw)) - ix;

                  // Filter stuff
                  if (o->filled)
                    {
                       iw -= l + r;
                       if (iw <= 0) break;
                    }

                  while ((int)idy < obj->cur->geometry.h)
                    {
                       int dobreak_h = 0;

                       iy = idy;
                       if ((o->cur->fill.h == obj->cur->geometry.h) &&
                           (o->cur->fill.y == 0))
                         {
                            ih = obj->cur->geometry.h;
                            dobreak_h = 1;
                         }
                       else
                         ih = ((int)(idy + idh)) - iy;

                       // Filter stuff
                       if (o->filled)
                         {
                            ih -= t + b;
                            if (ih <= 0) break;
                         }

                       if ((o->cur->border.l == 0) &&
                           (o->cur->border.r == 0) &&
                           (o->cur->border.t == 0) &&
                           (o->cur->border.b == 0) &&
                           (o->cur->border.fill != 0))
                         {
                            _draw_image
                              (obj, output, context, surface, pixels,
                               0, 0,
                               imagew, imageh,
                               offx + ix,
                               offy + iy,
                               iw, ih,
                               o->cur->smooth_scale,
                               do_async);
                         }
                       else
                         {
                            int inx, iny, inw, inh, outx, outy, outw, outh;
                            int bl, br, bt, bb, bsl, bsr, bst, bsb;
                            int imw, imh, ox, oy;

                            ox = offx + ix;
                            oy = offy + iy;
                            imw = imagew;
                            imh = imageh;
                            bl = o->cur->border.l;
                            br = o->cur->border.r;
                            bt = o->cur->border.t;
                            bb = o->cur->border.b;
                            // fix impossible border settings if img pixels not enough
                            if ((bl + br) > imw)
                              {
                                 if ((bl + br) > 0)
                                   {
                                      bl = (bl * imw) / (bl + br);
                                      br = imw - bl;
                                   }
                              }
                            if ((bt + bb) > imh)
                              {
                                 if ((bt + bb) > 0)
                                   {
                                      bt = (bt * imh) / (bt + bb);
                                      bb = imh - bt;
                                   }
                              }
                            if (o->cur->border.scale != 1.0)
                              {
                                 bsl = ((double)bl * o->cur->border.scale);
                                 bsr = ((double)br * o->cur->border.scale);
                                 bst = ((double)bt * o->cur->border.scale);
                                 bsb = ((double)bb * o->cur->border.scale);
                              }
                            else
                              {
                                  bsl = bl; bsr = br; bst = bt; bsb = bb;
                              }
                            // adjust output border rendering if it doesnt fit
                            if ((bsl + bsr) > iw)
                              {
                                 int b0 = bsl, b1 = bsr;

                                 if ((bsl + bsr) > 0)
                                   {
                                      bsl = (bsl * iw) / (bsl + bsr);
                                      bsr = iw - bsl;
                                   }
                                 if (b0 > 0) bl = (bl * bsl) / b0;
                                 else bl = 0;
                                 if (b1 > 0) br = (br * bsr) / b1;
                                 else br = 0;
                              }
                            if ((bst + bsb) > ih)
                              {
                                 int b0 = bst, b1 = bsb;

                                 if ((bst + bsb) > 0)
                                   {
                                      bst = (bst * ih) / (bst + bsb);
                                      bsb = ih - bst;
                                   }
                                 if (b0 > 0) bt = (bt * bst) / b0;
                                 else bt = 0;
                                 if (b1 > 0) bb = (bb * bsb) / b1;
                                 else bb = 0;
                              }
                            // #--.
                            // |  |
                            // '--'
                            inx = 0; iny = 0;
                            inw = bl; inh = bt;
                            outx = ox; outy = oy;
                            outw = bsl; outh = bst;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .##.
                            // |  |
                            // '--'
                            inx = bl; iny = 0;
                            inw = imw - bl - br; inh = bt;
                            outx = ox + bsl; outy = oy;
                            outw = iw - bsl - bsr; outh = bst;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--#
                            // |  |
                            // '--'
                            inx = imw - br; iny = 0;
                            inw = br; inh = bt;
                            outx = ox + iw - bsr; outy = oy;
                            outw = bsr; outh = bst;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--.
                            // #  |
                            // '--'
                            inx = 0; iny = bt;
                            inw = bl; inh = imh - bt - bb;
                            outx = ox; outy = oy + bst;
                            outw = bsl; outh = ih - bst - bsb;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--.
                            // |##|
                            // '--'
                            if (o->cur->border.fill > EVAS_BORDER_FILL_NONE)
                              {
                                 inx = bl; iny = bt;
                                 inw = imw - bl - br; inh = imh - bt - bb;
                                 outx = ox + bsl; outy = oy + bst;
                                 outw = iw - bsl - bsr; outh = ih - bst - bsb;
                                 if ((o->cur->border.fill == EVAS_BORDER_FILL_SOLID) &&
                                     (obj->cur->cache.clip.a == 255) &&
                                     (!obj->clip.mask) &&
                                     (obj->cur->render_op == EVAS_RENDER_BLEND))
                                   {
                                      ENFN->context_render_op_set(output, context, EVAS_RENDER_COPY);
                                      _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                                      ENFN->context_render_op_set(output, context, obj->cur->render_op);
                                   }
                                 else
                                   _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                              }
                            // .--.
                            // |  #
                            // '--'
                            inx = imw - br; iny = bt;
                            inw = br; inh = imh - bt - bb;
                            outx = ox + iw - bsr; outy = oy + bst;
                            outw = bsr; outh = ih - bst - bsb;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--.
                            // |  |
                            // #--'
                            inx = 0; iny = imh - bb;
                            inw = bl; inh = bb;
                            outx = ox; outy = oy + ih - bsb;
                            outw = bsl; outh = bsb;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--.
                            // |  |
                            // '##'
                            inx = bl; iny = imh - bb;
                            inw = imw - bl - br; inh = bb;
                            outx = ox + bsl; outy = oy + ih - bsb;
                            outw = iw - bsl - bsr; outh = bsb;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                            // .--.
                            // |  |
                            // '--#
                            inx = imw - br; iny = imh - bb;
                            inw = br; inh = bb;
                            outx = ox + iw - bsr; outy = oy + ih - bsb;
                            outw = bsr; outh = bsb;
                            _draw_image(obj, output, context, surface, pixels, inx, iny, inw, inh, outx, outy, outw, outh, o->cur->smooth_scale, do_async);
                         }
                       idy += idh;
                       if (dobreak_h) break;
                    }
                  idx += idw;
                  idy = ydy;
                  if (dobreak_w) break;
               }
          }
     }
}

static void
evas_object_image_render_pre(Evas_Object *eo_obj,
			     Evas_Object_Protected_Data *obj,
			     void *type_private_data)
{
   Evas_Image_Data *o = type_private_data;
   int is_v = 0, was_v = 0;

   /* dont pre-render the obj twice! */
   if (obj->pre_render_done) return;
   obj->pre_render_done = EINA_TRUE;
   /* pre-render phase. this does anything an object needs to do just before */
   /* rendering. this could mean loading the image data, retrieving it from */
   /* elsewhere, decoding video etc. */
   /* then when this is done the object needs to figure if it changed and */
   /* if so what and where and add the appropriate redraw rectangles */
   Evas_Public_Data *e = obj->layer->evas;

   if ((o->cur->fill.w < 1) || (o->cur->fill.h < 1)) return;

   /* if someone is clipping this obj - go calculate the clipper */
   if (obj->cur->clipper)
     {
	if (obj->cur->cache.clip.dirty)
	  evas_object_clip_recalc(obj->cur->clipper);
	obj->cur->clipper->func->render_pre(obj->cur->clipper->object,
					    obj->cur->clipper,
					    obj->cur->clipper->private_data);
     }
   /* Proxy: Do it early */
   if (o->cur->source)
     {
        Evas_Object_Protected_Data *source = eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
        if (source->proxy->redraw || source->changed)
          {
             /* XXX: Do I need to sort out the map here? */
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
     }
   else if (o->cur->scene)
     {
        Evas_Canvas3D_Scene *scene = o->cur->scene;
        Eina_Bool dirty;

        dirty = evas_canvas3d_object_dirty_get(scene, EVAS_CANVAS3D_STATE_ANY);
        if (dirty)
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
     }

   /* now figure what changed and add draw rects */
   /* if it just became visible or invisible */
   is_v = evas_object_is_visible(eo_obj, obj);
   was_v = evas_object_was_visible(eo_obj, obj);
   if (is_v != was_v)
     {
        evas_object_render_pre_visible_change(&e->clip_changes, eo_obj, is_v, was_v);
        goto done;
     }
   if (obj->changed_map || obj->changed_src_visible)
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   /* it's not visible - we accounted for it appearing or not so just abort */
   if (!is_v) goto done;
   /* clipper changed this is in addition to anything else for obj */
   if (was_v)
     evas_object_render_pre_clipper_change(&e->clip_changes, eo_obj);
   /* if we restacked (layer or just within a layer) and don't clip anyone */
   if (obj->restack)
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed color */
   if ((obj->cur->color.r != obj->prev->color.r) ||
       (obj->cur->color.g != obj->prev->color.g) ||
       (obj->cur->color.b != obj->prev->color.b) ||
       (obj->cur->color.a != obj->prev->color.a) ||
       (obj->cur->cache.clip.r != obj->prev->cache.clip.r) ||
       (obj->cur->cache.clip.g != obj->prev->cache.clip.g) ||
       (obj->cur->cache.clip.b != obj->prev->cache.clip.b) ||
       (obj->cur->cache.clip.a != obj->prev->cache.clip.a))
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed render op */
   if (obj->cur->render_op != obj->prev->render_op)
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   /* if it changed anti_alias */
   if (obj->cur->anti_alias != obj->prev->anti_alias)
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   if (o->changed)
     {
        if (((o->cur->u.file) && (!o->prev->u.file)) ||
            ((!o->cur->u.file) && (o->prev->u.file)) ||
            ((o->cur->key) && (!o->prev->key)) ||
            ((!o->cur->key) && (o->prev->key))
           )
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
        if ((o->cur->image.w != o->prev->image.w) ||
            (o->cur->image.h != o->prev->image.h) ||
            (o->cur->has_alpha != o->prev->has_alpha) ||
            (o->cur->cspace != o->prev->cspace) ||
            (o->cur->smooth_scale != o->prev->smooth_scale))
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
        if ((o->cur->border.l != o->prev->border.l) ||
            (o->cur->border.r != o->prev->border.r) ||
            (o->cur->border.t != o->prev->border.t) ||
            (o->cur->border.b != o->prev->border.b) ||
            (o->cur->border.fill != o->prev->border.fill) ||
            (o->cur->border.scale != o->prev->border.scale))
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
        if (o->dirty_pixels && ENFN->image_native_get)
          {
             /* Evas GL surfaces have historically required only the dirty
              * pixel to trigger a redraw (call to pixels_get). Other kinds
              * of surfaces must add data update regions. */
             Evas_Native_Surface *ns;
             ns = ENFN->image_native_get(ENDT, o->engine_data);
             if (ns && (ns->type == EVAS_NATIVE_SURFACE_EVASGL))
               {
                  evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
                  goto done;
               }
          }
        if (o->cur->frame != o->prev->frame)
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
        if (o->cur->orient != o->prev->orient)
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }

     }
   if (((obj->cur->geometry.x != obj->prev->geometry.x) ||
        (obj->cur->geometry.y != obj->prev->geometry.y) ||
        (obj->cur->geometry.w != obj->prev->geometry.w) ||
        (obj->cur->geometry.h != obj->prev->geometry.h))
      )
     {
        evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
        goto done;
     }
   if (o->changed)
     {
        if ((o->cur->fill.x != o->prev->fill.x) ||
            (o->cur->fill.y != o->prev->fill.y) ||
            (o->cur->fill.w != o->prev->fill.w) ||
            (o->cur->fill.h != o->prev->fill.h))
          {
             evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj, obj);
             goto done;
          }
        if (o->pixels->pixel_updates)
          {
             if ((o->cur->border.l == 0) &&
                 (o->cur->border.r == 0) &&
                 (o->cur->border.t == 0) &&
                 (o->cur->border.b == 0) &&
                 (o->cur->image.w > 0) &&
                 (o->cur->image.h > 0) &&
                 (!((obj->map->cur.map) && (obj->map->cur.usemap))))
               {
                  Eina_Rectangle *rr;

                  if ((!o->cur->has_alpha) &&
                      (evas_object_is_opaque(eo_obj, obj)) &&
                      (obj->cur->color.a == 255))
                    {
                       Evas_Coord x, y, w, h;

                       x = obj->cur->cache.clip.x;
                       y = obj->cur->cache.clip.y;
                       w = obj->cur->cache.clip.w;
                       h = obj->cur->cache.clip.h;
                       if (obj->cur->clipper)
                         {
                            if (obj->cur->clipper->cur->cache.clip.a != 255)
                              w = 0;
                            else
                              {
                                 RECTS_CLIP_TO_RECT(x, y, w, h,
                                                    obj->cur->clipper->cur->cache.clip.x,
                                                    obj->cur->clipper->cur->cache.clip.y,
                                                    obj->cur->clipper->cur->cache.clip.w,
                                                    obj->cur->clipper->cur->cache.clip.h);
                              }
                         }
                       if ((w > 0) && (h > 0))
                         e->engine.func->output_redraws_rect_del(e->engine.data.output,
                                                                 x + e->framespace.x,
                                                                 y + e->framespace.y,
                                                                 w, h);
                    }
                  EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
                    {
                       EINA_LIST_FREE(pixi_write->pixel_updates, rr)
                         {
                            Evas_Coord idw, idh, idx, idy;
                            int x, y, w, h;
                            e->engine.func->image_dirty_region(e->engine.data.output, o->engine_data, rr->x, rr->y, rr->w, rr->h);

                            idx = evas_object_image_figure_x_fill(eo_obj, obj, o->cur->fill.x, o->cur->fill.w, &idw);
                            idy = evas_object_image_figure_y_fill(eo_obj, obj, o->cur->fill.y, o->cur->fill.h, &idh);

                            if (idw < 1) idw = 1;
                            if (idh < 1) idh = 1;
                            if (idx > 0) idx -= idw;
                            if (idy > 0) idy -= idh;
                            while (idx < obj->cur->geometry.w)
                              {
                                 Evas_Coord ydy;

                                 ydy = idy;
                                 x = idx;
                                 w = ((int)(idx + idw)) - x;
                                 while (idy < obj->cur->geometry.h)
                                   {
                                      Eina_Rectangle r;

                                      y = idy;
                                      h = ((int)(idy + idh)) - y;

                                      r.x = (rr->x * w) / o->cur->image.w;
                                      r.y = (rr->y * h) / o->cur->image.h;
                                      r.w = ((rr->w * w) + (o->cur->image.w * 2) - 1) / o->cur->image.w;
                                      r.h = ((rr->h * h) + (o->cur->image.h * 2) - 1) / o->cur->image.h;
                                      r.x += obj->cur->geometry.x + x;
                                      r.y += obj->cur->geometry.y + y;
                                      RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h,
                                                         obj->cur->cache.clip.x, obj->cur->cache.clip.y,
                                                         obj->cur->cache.clip.w, obj->cur->cache.clip.h);
                                      evas_add_rect(&e->clip_changes, r.x, r.y, r.w, r.h);
                                      idy += h;
                                   }
                                 idx += idw;
                                 idy = ydy;
                              }
                            eina_rectangle_free(rr);
                         }
                    }
                  EINA_COW_PIXEL_WRITE_END(o, pixi_write);
               }
             else
               {
                  if ((o->cur->image.w > 0) &&
                      (o->cur->image.h > 0) &&
                      (o->cur->image.w == obj->cur->geometry.w) &&
                      (o->cur->image.h == obj->cur->geometry.h) &&
                      (o->cur->fill.x == 0) &&
                      (o->cur->fill.y == 0) &&
                      (o->cur->fill.w == o->cur->image.w) &&
                      (o->cur->fill.h == o->cur->image.h) &&
                      (!((obj->map->cur.map) && (obj->map->cur.usemap))))
                    {
                       Eina_Rectangle *rr;

                       if ((!o->cur->has_alpha) &&
                           (evas_object_is_opaque(eo_obj, obj)) &&
                           (obj->cur->color.a == 255))
                         {
                            Evas_Coord x, y, w, h;

                            x = obj->cur->cache.clip.x;
                            y = obj->cur->cache.clip.y;
                            w = obj->cur->cache.clip.w;
                            h = obj->cur->cache.clip.h;
                            if (obj->cur->clipper)
                              {
                                 if (obj->cur->clipper->cur->cache.clip.a != 255)
                                   w = 0;
                                 else
                                   {
                                      RECTS_CLIP_TO_RECT(x, y, w, h,
                                                         obj->cur->clipper->cur->cache.clip.x,
                                                         obj->cur->clipper->cur->cache.clip.y,
                                                         obj->cur->clipper->cur->cache.clip.w,
                                                         obj->cur->clipper->cur->cache.clip.h);
                                   }
                              }
                            if ((w > 0) && (h > 0))
                              e->engine.func->output_redraws_rect_del(e->engine.data.output,
                                                                      x + e->framespace.x,
                                                                      y + e->framespace.y,
                                                                      w, h);
                         }
                       else if ((o->cur->border.fill == EVAS_BORDER_FILL_SOLID) &&
                                (obj->cur->color.a == 255))
                         {
                            Evas_Coord x, y, w, h;

                            x = obj->cur->geometry.x + o->cur->border.l;
                            y = obj->cur->geometry.y + o->cur->border.t;
                            w = obj->cur->geometry.w - o->cur->border.l - o->cur->border.r;
                            h = obj->cur->geometry.h - o->cur->border.t - o->cur->border.b;
                            if (obj->cur->clipper)
                              {
                                 if (obj->cur->clipper->cur->cache.clip.a != 255)
                                   w = 0;
                                 else
                                   {
                                      RECTS_CLIP_TO_RECT(x, y, w, h,
                                                         obj->cur->clipper->cur->cache.clip.x,
                                                         obj->cur->clipper->cur->cache.clip.y,
                                                         obj->cur->clipper->cur->cache.clip.w,
                                                         obj->cur->clipper->cur->cache.clip.h);
                                   }
                              }
                            if ((w > 0) && (h > 0))
                              e->engine.func->output_redraws_rect_del(e->engine.data.output,
                                                                      x + e->framespace.x,
                                                                      y + e->framespace.y,
                                                                      w, h);
                         }
                       EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
                         {
                            EINA_LIST_FREE(pixi_write->pixel_updates, rr)
                              {
                                 Eina_Rectangle r;

                                 e->engine.func->image_dirty_region(e->engine.data.output, o->engine_data, rr->x, rr->y, rr->w, rr->h);
                                 r.x = rr->x;
                                 r.y = rr->y;
                                 r.w = rr->w;
                                 r.h = rr->h;
                                 r.x += obj->cur->geometry.x;
                                 r.y += obj->cur->geometry.y;
                                 RECTS_CLIP_TO_RECT(r.x, r.y, r.w, r.h,
                                                    obj->cur->cache.clip.x, obj->cur->cache.clip.y,
                                                    obj->cur->cache.clip.w, obj->cur->cache.clip.h);
                                 evas_add_rect(&e->clip_changes, r.x, r.y, r.w, r.h);
                                 eina_rectangle_free(rr);
                              }
                         }
                       EINA_COW_PIXEL_WRITE_END(o, pixi_write);
                    }
                  else
                    {
                       Eina_Rectangle *r;

                       EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
                         {
                            EINA_LIST_FREE(pixi_write->pixel_updates, r)
                              eina_rectangle_free(r);
                         }
                       EINA_COW_PIXEL_WRITE_END(o, pixi_write);
                       e->engine.func->image_dirty_region(e->engine.data.output, o->engine_data, 0, 0, o->cur->image.w, o->cur->image.h);

                       evas_object_render_pre_prev_cur_add(&e->clip_changes, eo_obj,
                                                           obj);
                    }
               }
             goto done;
          }
     }
   /* it obviously didn't change - add a NO obscure - this "unupdates"  this */
   /* area so if there were updates for it they get wiped. don't do it if we */
   /* aren't fully opaque and we are visible */
   if (evas_object_is_opaque(eo_obj, obj))
     {
        Evas_Coord x, y, w, h;

        x = obj->cur->cache.clip.x;
        y = obj->cur->cache.clip.y;
        w = obj->cur->cache.clip.w;
        h = obj->cur->cache.clip.h;
        if (obj->cur->clipper)
          {
             RECTS_CLIP_TO_RECT(x, y, w, h,
                                obj->cur->clipper->cur->cache.clip.x,
                                obj->cur->clipper->cur->cache.clip.y,
                                obj->cur->clipper->cur->cache.clip.w,
                                obj->cur->clipper->cur->cache.clip.h);
          }
        e->engine.func->output_redraws_rect_del(e->engine.data.output,
                                                x + e->framespace.x,
                                                y + e->framespace.y,
                                                w, h);
     }
   done:
   evas_object_render_pre_effect_updates(&e->clip_changes, eo_obj, is_v, was_v);
   if (o->pixels->pixel_updates)
     {
        Eina_Rectangle *rr;

        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          {
             EINA_LIST_FREE(pixi_write->pixel_updates, rr)
               {
                  eina_rectangle_free(rr);
               }
          }
        EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }
}

static void
evas_object_image_render_post(Evas_Object *eo_obj,
			      Evas_Object_Protected_Data *obj EINA_UNUSED,
			      void *type_private_data)
{
   Evas_Image_Data *o = type_private_data;
   Eina_Rectangle *r;

   /* this moves the current data to the previous state parts of the object */
   /* in whatever way is safest for the object. also if we don't need object */
   /* data anymore we can free it if the object deems this is a good idea */
   /* remove those pesky changes */
   evas_object_clip_changes_clean(eo_obj);

   if (o->pixels->pixel_updates)
     {
        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          {
             EINA_LIST_FREE(pixi_write->pixel_updates, r)
               eina_rectangle_free(r);
          }
        EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }

   /* move cur to prev safely for object data */
   evas_object_cur_prev(eo_obj);
   eina_cow_memcpy(evas_object_image_state_cow, (const Eina_Cow_Data **) &o->prev, o->cur);
   /* FIXME: copy strings across */
}

static unsigned int evas_object_image_id_get(Evas_Object *eo_obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return 0;
   return MAGIC_OBJ_IMAGE;
}

static unsigned int evas_object_image_visual_id_get(Evas_Object *eo_obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return 0;
   return MAGIC_OBJ_IMAGE;
}

static void *evas_object_image_engine_data_get(Evas_Object *eo_obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (!o) return NULL;
   return o->engine_data;
}

static int
evas_object_image_is_opaque(Evas_Object *eo_obj EINA_UNUSED,
			    Evas_Object_Protected_Data *obj,
			    void *type_private_data)
{
   /* this returns 1 if the internal object data implies that the object is */
   /* currently fully opaque over the entire rectangle it occupies */
/*  disable caching due tyo maps screwing with this
    o->cur.opaque_valid = 0;*/
   Evas_Image_Data *o = type_private_data;

   if (o->cur->opaque_valid)
     {
        if (!o->cur->opaque) return 0;
     }
   else
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->opaque = 0;
             state_write->opaque_valid = 1;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

        if ((o->cur->fill.w < 1) || (o->cur->fill.h < 1))
          return o->cur->opaque;
        if (((o->cur->border.l != 0) ||
             (o->cur->border.r != 0) ||
             (o->cur->border.t != 0) ||
             (o->cur->border.b != 0)) &&
            (!o->cur->border.fill))
          return o->cur->opaque;
        if (!o->engine_data)
          return o->cur->opaque;
        if (o->has_filter)
          return o->cur->opaque;

        // FIXME: use proxy
        if (o->cur->source)
          {
             Evas_Object_Protected_Data *cur_source = eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS);
             EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
               {
                  state_write->opaque = evas_object_is_opaque(o->cur->source, cur_source);
               }
             EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
             return o->cur->opaque; /* FIXME: Should go poke at the object */
          }
        if (o->cur->has_alpha)
          return o->cur->opaque;

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->opaque = 1;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);
     }

   if ((obj->map->cur.map) && (obj->map->cur.usemap))
     {
        Evas_Map *m = obj->map->cur.map;

        if ((m->points[0].a == 255) &&
            (m->points[1].a == 255) &&
            (m->points[2].a == 255) &&
            (m->points[3].a == 255))
          {
             if (
                 ((m->points[0].x == m->points[3].x) &&
                     (m->points[1].x == m->points[2].x) &&
                     (m->points[0].y == m->points[1].y) &&
                     (m->points[2].y == m->points[3].y))
                 ||
                 ((m->points[0].x == m->points[1].x) &&
                     (m->points[2].x == m->points[3].x) &&
                     (m->points[0].y == m->points[3].y) &&
                     (m->points[1].y == m->points[2].y))
                )
               {
                  if ((m->points[0].x == obj->cur->geometry.x) &&
                      (m->points[0].y == obj->cur->geometry.y) &&
                      (m->points[2].x == (obj->cur->geometry.x + obj->cur->geometry.w)) &&
                      (m->points[2].y == (obj->cur->geometry.y + obj->cur->geometry.h)))
                    return o->cur->opaque;
               }
          }

        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->opaque = 0;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

        return o->cur->opaque;
     }
   if (obj->cur->render_op == EVAS_RENDER_COPY)
     {
        EINA_COW_IMAGE_STATE_WRITE_BEGIN(o, state_write)
          {
             state_write->opaque = 1;
          }
        EINA_COW_IMAGE_STATE_WRITE_END(o, state_write);

        return o->cur->opaque;
     }
   return o->cur->opaque;
}

static int
evas_object_image_was_opaque(Evas_Object *eo_obj EINA_UNUSED,
			     Evas_Object_Protected_Data *obj,
			     void *type_private_data)
{
   Evas_Image_Data *o = type_private_data;

   /* this returns 1 if the internal object data implies that the object was */
   /* previously fully opaque over the entire rectangle it occupies */
   if (o->prev->opaque_valid)
     {
        if (!o->prev->opaque) return 0;
     }
   else
     {
        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->opaque = 0;
             state_write->opaque_valid = 1;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);

        if ((o->prev->fill.w < 1) || (o->prev->fill.h < 1))
          return o->prev->opaque;
        if (((o->prev->border.l != 0) ||
             (o->prev->border.r != 0) ||
             (o->prev->border.t != 0) ||
             (o->prev->border.b != 0)) &&
            (!o->prev->border.fill))
          return o->prev->opaque;
        if (!o->engine_data)
          return o->prev->opaque;

        // FIXME: use proxy
        if (o->prev->source)
          return o->prev->opaque; /* FIXME: Should go poke at the object */
        if (o->prev->has_alpha)
          return o->prev->opaque;
        if (o->has_filter)
          return o->cur->opaque;

        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->opaque = 1;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);
     }
   if (obj->map->prev.usemap)
     {
        Evas_Map *m = obj->map->prev.map;

        if ((m->points[0].a == 255) &&
            (m->points[1].a == 255) &&
            (m->points[2].a == 255) &&
            (m->points[3].a == 255))
          {
             if (
                 ((m->points[0].x == m->points[3].x) &&
                     (m->points[1].x == m->points[2].x) &&
                     (m->points[0].y == m->points[1].y) &&
                     (m->points[2].y == m->points[3].y))
                 ||
                 ((m->points[0].x == m->points[1].x) &&
                     (m->points[2].x == m->points[3].x) &&
                     (m->points[0].y == m->points[3].y) &&
                     (m->points[1].y == m->points[2].y))
                )
               {
                  if ((m->points[0].x == obj->prev->geometry.x) &&
                      (m->points[0].y == obj->prev->geometry.y) &&
                      (m->points[2].x == (obj->prev->geometry.x + obj->prev->geometry.w)) &&
                      (m->points[2].y == (obj->prev->geometry.y + obj->prev->geometry.h)))
                    return o->prev->opaque;
               }
          }

        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->opaque = 0;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);

        return o->prev->opaque;
     }
   if (obj->prev->render_op == EVAS_RENDER_COPY)
     {
        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->opaque = 1;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);

        return o->prev->opaque;
     }
   if (obj->prev->render_op != EVAS_RENDER_BLEND)
     {
        EINA_COW_WRITE_BEGIN(evas_object_image_state_cow, o->prev, Evas_Object_Image_State, state_write)
          {
             state_write->opaque = 0;
          }
        EINA_COW_WRITE_END(evas_object_image_state_cow, o->prev, state_write);

        return o->prev->opaque;
     }
   return o->prev->opaque;
}

static int
evas_object_image_is_inside(Evas_Object *eo_obj,
			    Evas_Object_Protected_Data *obj,
			    void *type_private_data,
			    Evas_Coord px, Evas_Coord py)
{
   Evas_Image_Data *o = type_private_data;
   int imagew, imageh, uvw, uvh;
   void *pixels;
   Evas_Func *eng = ENFN;
   int is_inside = 0;

   /* the following code is similar to evas_object_image_render(), but doesn't
    * draw, just get the pixels so we can check the transparency.
    */
   Evas_Object_Protected_Data *source =
      (o->cur->source ?
       eo_data_scope_get(o->cur->source, EFL_CANVAS_OBJECT_CLASS):
       NULL);

   if (o->cur->scene)
     {
        _evas_image_3d_render(obj->layer->evas->evas, eo_obj, obj, o, o->cur->scene);
        pixels = obj->data_3d->surface;
        imagew = obj->data_3d->w;
        imageh = obj->data_3d->h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (!o->cur->source)
     {
        pixels = o->engine_data;
        imagew = o->cur->image.w;
        imageh = o->cur->image.h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (source->proxy->surface && !source->proxy->redraw)
     {
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
        uvw = imagew;
        uvh = imageh;
     }
   else if (source->type == o_type &&
            ((Evas_Image_Data *)eo_data_scope_get(o->cur->source, MY_CLASS))->engine_data)
     {
        Evas_Image_Data *oi;
        oi = eo_data_scope_get(o->cur->source, MY_CLASS);
        pixels = oi->engine_data;
        imagew = oi->cur->image.w;
        imageh = oi->cur->image.h;
        uvw = source->cur->geometry.w;
        uvh = source->cur->geometry.h;
     }
   else
     {
        o->proxyrendering = EINA_TRUE;
        evas_render_proxy_subrender(obj->layer->evas->evas, o->cur->source,
                                    eo_obj, obj, EINA_FALSE);
        pixels = source->proxy->surface;
        imagew = source->proxy->w;
        imageh = source->proxy->h;
        uvw = imagew;
        uvh = imageh;
        o->proxyrendering = EINA_FALSE;
     }

   if (pixels)
     {
        Evas_Coord idw, idh, idx, idy;
        int ix, iy, iw, ih;

        /* TODO: not handling o->dirty_pixels && o->pixels->func.get_pixels,
         * should we handle it now or believe they were done in the last render?
         */
        if (o->dirty_pixels)
          {
             if (o->pixels->func.get_pixels)
               {
                  ERR("dirty_pixels && get_pixels not supported");
               }
          }

        /* TODO: not handling map, need to apply map to point */
        if ((obj->map->cur.map) && (obj->map->cur.map->count > 3) && (obj->map->cur.usemap))
          {
             evas_object_map_update(eo_obj, 0, 0, imagew, imageh, uvw, uvh);

             ERR("map not supported");
          }
        else
          {
             idx = evas_object_image_figure_x_fill(eo_obj, obj, o->cur->fill.x, o->cur->fill.w, &idw);
             idy = evas_object_image_figure_y_fill(eo_obj, obj, o->cur->fill.y, o->cur->fill.h, &idh);
             if (idw < 1) idw = 1;
             if (idh < 1) idh = 1;
             if (idx > 0) idx -= idw;
             if (idy > 0) idy -= idh;
             while ((int)idx < obj->cur->geometry.w)
               {
                  Evas_Coord ydy;
                  int dobreak_w = 0;
                  ydy = idy;
                  ix = idx;
                  if ((o->cur->fill.w == obj->cur->geometry.w) &&
                      (o->cur->fill.x == 0))
                    {
                       dobreak_w = 1;
                       iw = obj->cur->geometry.w;
                    }
                  else
                    iw = ((int)(idx + idw)) - ix;
                  while ((int)idy < obj->cur->geometry.h)
                    {
                       int dobreak_h = 0;

                       iy = idy;
                       if ((o->cur->fill.h == obj->cur->geometry.h) &&
                           (o->cur->fill.y == 0))
                         {
                            ih = obj->cur->geometry.h;
                            dobreak_h = 1;
                         }
                       else
                         ih = ((int)(idy + idh)) - iy;
                       if ((o->cur->border.l == 0) &&
                           (o->cur->border.r == 0) &&
                           (o->cur->border.t == 0) &&
                           (o->cur->border.b == 0) &&
                           (o->cur->border.fill != 0))
                         {
                            /* NOTE: render handles cserve2 here,
                             * we don't need to
                             */
                              {
                                 DATA8 alpha = 0;

                                 if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                          0, 0,
                                                          imagew, imageh,
                                                          obj->cur->geometry.x + ix,
                                                          obj->cur->geometry.y + iy,
                                                          iw, ih))
                                   {
                                      is_inside = alpha > 0;
                                      dobreak_h = 1;
                                      dobreak_w = 1;
                                      break;
                                   }
                              }
                         }
                       else
                         {
                            int inx, iny, inw, inh, outx, outy, outw, outh;
                            int bl, br, bt, bb, bsl, bsr, bst, bsb;
                            int imw, imh, ox, oy;
                            DATA8 alpha = 0;

                            ox = obj->cur->geometry.x + ix;
                            oy = obj->cur->geometry.y + iy;
                            imw = imagew;
                            imh = imageh;
                            bl = o->cur->border.l;
                            br = o->cur->border.r;
                            bt = o->cur->border.t;
                            bb = o->cur->border.b;
                            if ((bl + br) > iw)
                              {
                                 bl = iw / 2;
                                 br = iw - bl;
                              }
                            if ((bl + br) > imw)
                              {
                                 bl = imw / 2;
                                 br = imw - bl;
                              }
                            if ((bt + bb) > ih)
                              {
                                 bt = ih / 2;
                                 bb = ih - bt;
                              }
                            if ((bt + bb) > imh)
                              {
                                 bt = imh / 2;
                                 bb = imh - bt;
                              }
                            if (o->cur->border.scale != 1.0)
                              {
                                 bsl = ((double)bl * o->cur->border.scale);
                                 bsr = ((double)br * o->cur->border.scale);
                                 bst = ((double)bt * o->cur->border.scale);
                                 bsb = ((double)bb * o->cur->border.scale);
                              }
                            else
                              {
                                  bsl = bl; bsr = br; bst = bt; bsb = bb;
                              }
                            // #--
                            // |
                            inx = 0; iny = 0;
                            inw = bl; inh = bt;
                            outx = ox; outy = oy;
                            outw = bsl; outh = bst;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }

                            // .##
                            // |
                            inx = bl; iny = 0;
                            inw = imw - bl - br; inh = bt;
                            outx = ox + bsl; outy = oy;
                            outw = iw - bsl - bsr; outh = bst;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            // --#
                            //   |
                            inx = imw - br; iny = 0;
                            inw = br; inh = bt;
                            outx = ox + iw - bsr; outy = oy;
                            outw = bsr; outh = bst;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            // .--
                            // #
                            inx = 0; iny = bt;
                            inw = bl; inh = imh - bt - bb;
                            outx = ox; outy = oy + bst;
                            outw = bsl; outh = ih - bst - bsb;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            // .--.
                            // |##|
                            if (o->cur->border.fill > EVAS_BORDER_FILL_NONE)
                              {
                                 inx = bl; iny = bt;
                                 inw = imw - bl - br; inh = imh - bt - bb;
                                 outx = ox + bsl; outy = oy + bst;
                                 outw = iw - bsl - bsr; outh = ih - bst - bsb;
                                 if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                          inx, iny, inw, inh,
                                                          outx, outy, outw, outh))
                                   {
                                      is_inside = alpha > 0;
                                      dobreak_h = 1;
                                      dobreak_w = 1;
                                      break;
                                   }
                              }
                            // --.
                            //   #
                            inx = imw - br; iny = bt;
                            inw = br; inh = imh - bt - bb;
                            outx = ox + iw - bsr; outy = oy + bst;
                            outw = bsr; outh = ih - bst - bsb;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            // |
                            // #--
                            inx = 0; iny = imh - bb;
                            inw = bl; inh = bb;
                            outx = ox; outy = oy + ih - bsb;
                            outw = bsl; outh = bsb;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            // |
                            // .##
                            inx = bl; iny = imh - bb;
                            inw = imw - bl - br; inh = bb;
                            outx = ox + bsl; outy = oy + ih - bsb;
                            outw = iw - bsl - bsr; outh = bsb;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                            //   |
                            // --#
                            inx = imw - br; iny = imh - bb;
                            inw = br; inh = bb;
                            outx = ox + iw - bsr; outy = oy + ih - bsb;
                            outw = bsr; outh = bsb;
                            if (eng->pixel_alpha_get(pixels, px, py, &alpha,
                                                     inx, iny, inw, inh,
                                                     outx, outy, outw, outh))
                              {
                                 is_inside = alpha > 0;
                                 dobreak_h = 1;
                                 dobreak_w = 1;
                                 break;
                              }
                         }
                       idy += idh;
                       if (dobreak_h) break;
                    }
                  idx += idw;
                  idy = ydy;
                  if (dobreak_w) break;
               }
          }
     }

   return is_inside;
}

static int
evas_object_image_has_opaque_rect(Evas_Object *eo_obj EINA_UNUSED,
				  Evas_Object_Protected_Data *obj,
				  void *type_private_data)
{
   Evas_Image_Data *o = type_private_data;

   if ((obj->map->cur.map) && (obj->map->cur.usemap)) return 0;
   if (((o->cur->border.l | o->cur->border.r | o->cur->border.t | o->cur->border.b) != 0) &&
       (o->cur->border.fill == EVAS_BORDER_FILL_SOLID) &&
       (obj->cur->render_op == EVAS_RENDER_BLEND) &&
       (obj->cur->cache.clip.a == 255) &&
       (o->cur->fill.x == 0) &&
       (o->cur->fill.y == 0) &&
       (o->cur->fill.w == obj->cur->geometry.w) &&
       (o->cur->fill.h == obj->cur->geometry.h)
       ) return 1;
   return 0;
}

static int
evas_object_image_get_opaque_rect(Evas_Object *eo_obj EINA_UNUSED,
				  Evas_Object_Protected_Data *obj,
				  void *type_private_data,
				  Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Image_Data *o = type_private_data;

   if (!o->cur->has_alpha)
     {
        *x = obj->cur->geometry.x;
        *y = obj->cur->geometry.y;
        *w = obj->cur->geometry.w;
        *h = obj->cur->geometry.h;
     }
   else if (o->cur->border.fill == EVAS_BORDER_FILL_SOLID)
     {
        *x = obj->cur->geometry.x + (o->cur->border.l * o->cur->border.scale);
        *y = obj->cur->geometry.y + (o->cur->border.t * o->cur->border.scale);
        *w = obj->cur->geometry.w - ((o->cur->border.l * o->cur->border.scale)
                                     + (o->cur->border.r * o->cur->border.scale));
        if (*w < 0) *w = 0;
        *h = obj->cur->geometry.h - ((o->cur->border.t * o->cur->border.scale)
                                     + (o->cur->border.b * o->cur->border.scale));
        if (*h < 0) *h = 0;
     }
   else
     {
        *w = 0;
        *h = 0;
     }
   return 1;
}

static int
evas_object_image_can_map(Evas_Object *obj EINA_UNUSED)
{
   return 1;
}

void *
_evas_image_data_convert_internal(Evas_Image_Data *o, void *data, Evas_Colorspace to_cspace)
{
   void *out = NULL;

   if (!data)
     return NULL;

   switch (o->cur->cspace)
     {
      case EVAS_COLORSPACE_ARGB8888:
         out = evas_common_convert_argb8888_to(data,
                                               o->cur->image.w,
                                               o->cur->image.h,
                                               o->cur->image.stride >> 2,
                                               o->cur->has_alpha,
                                               to_cspace);
         break;
      case EVAS_COLORSPACE_RGB565_A5P:
         out = evas_common_convert_rgb565_a5p_to(data,
                                                 o->cur->image.w,
                                                 o->cur->image.h,
                                                 o->cur->image.stride >> 1,
                                                 o->cur->has_alpha,
                                                 to_cspace);
         break;
      case EVAS_COLORSPACE_YCBCR422601_PL:
         out = evas_common_convert_yuv_422_601_to(data,
                                                  o->cur->image.w,
                                                   o->cur->image.h,
                                                   to_cspace);
          break;
        case EVAS_COLORSPACE_YCBCR422P601_PL:
          out = evas_common_convert_yuv_422P_601_to(data,
                                                    o->cur->image.w,
                                                    o->cur->image.h,
                                                    to_cspace);
          break;
        case EVAS_COLORSPACE_YCBCR420NV12601_PL:
          out = evas_common_convert_yuv_420_601_to(data,
                                                   o->cur->image.w,
                                                   o->cur->image.h,
                                                   to_cspace);
          break;
        case EVAS_COLORSPACE_YCBCR420TM12601_PL:
          out = evas_common_convert_yuv_420T_601_to(data,
                                                    o->cur->image.w,
                                                    o->cur->image.h,
                                                    to_cspace);
          break;
      case EVAS_COLORSPACE_AGRY88:
          out = evas_common_convert_agry88_to(data,
                                              o->cur->image.w,
                                              o->cur->image.h,
                                              o->cur->image.stride,
                                              o->cur->has_alpha,
                                              to_cspace);
          break;
      case EVAS_COLORSPACE_GRY8:
          out = evas_common_convert_gry8_to(data,
                                            o->cur->image.w,
                                            o->cur->image.h,
                                            o->cur->image.stride,
                                            o->cur->has_alpha,
                                            to_cspace);
          break;
        default:
          WRN("unknow colorspace: %i\n", o->cur->cspace);
          break;
     }

   return out;
}

static void
evas_object_image_filled_resize_listener(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *einfo EINA_UNUSED)
{
   Evas_Image_Data *o = eo_data_scope_get(obj, EFL_CANVAS_IMAGE_INTERNAL_CLASS);
   Evas_Coord w, h;

   efl_gfx_geometry_get(obj, NULL, NULL, &w, &h);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   _evas_image_fill_set(obj, o, 0, 0, w, h);
}

Eina_Bool
_evas_object_image_preloading_get(const Evas_Object *eo_obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   return o->preloading;
}

void
_evas_object_image_preloading_set(Evas_Object *eo_obj, Eina_Bool preloading)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   o->preloading = preloading;
}

void
_evas_object_image_preloading_check(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   if (ENFN->image_load_error_get)
      o->load_error = ENFN->image_load_error_get(ENDT, o->engine_data);
}

Evas_Object *
_evas_object_image_video_parent_get(Evas_Object *eo_obj)
{
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   return o->video_surface ? o->pixels->video.parent : NULL;
}

void
_evas_object_image_video_overlay_show(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   if (obj->cur->cache.clip.x != obj->prev->cache.clip.x ||
       obj->cur->cache.clip.y != obj->prev->cache.clip.y ||
       o->created || !o->video_visible)
     o->delayed.video_move = EINA_TRUE;

   if (obj->cur->cache.clip.w != obj->prev->cache.clip.w ||
       obj->cur->cache.clip.h != obj->prev->cache.clip.h ||
       o->created || !o->video_visible)
     o->delayed.video_resize = EINA_TRUE;

   if (!o->video_visible || o->created)
     {
        o->delayed.video_show = EINA_TRUE;
        o->delayed.video_hide = EINA_FALSE;
     }
   else
     {
        /* Cancel dirty on the image */
        Eina_Rectangle *r;

        o->dirty_pixels = EINA_FALSE;

        EINA_COW_PIXEL_WRITE_BEGIN(o, pixi_write)
          {
             EINA_LIST_FREE(pixi_write->pixel_updates, r)
               eina_rectangle_free(r);
          }
        EINA_COW_PIXEL_WRITE_END(o, pixi_write);
     }
   o->video_visible = EINA_TRUE;
   o->created = EINA_FALSE;
}

void
_evas_object_image_video_overlay_hide(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);

   if (o->video_visible || o->created)
     {
        o->delayed.video_hide = EINA_TRUE;
        o->delayed.video_show = EINA_FALSE;
     }
   if (evas_object_is_visible(eo_obj, obj))
     o->pixels->video.update_pixels(o->pixels->video.data, eo_obj, &o->pixels->video);
   o->video_visible = EINA_FALSE;
   o->created = EINA_FALSE;
}

void
_evas_object_image_video_overlay_do(Evas_Object *eo_obj)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EFL_CANVAS_OBJECT_CLASS);
   Evas_Image_Data *o = eo_data_scope_get(eo_obj, MY_CLASS);
   Evas_Public_Data *e = obj->layer->evas;

   if (o->delayed.video_move)
     o->pixels->video.move(o->pixels->video.data, eo_obj, &o->pixels->video,
                           obj->cur->cache.clip.x + e->framespace.x,
                           obj->cur->cache.clip.y + e->framespace.y);

   if (o->delayed.video_resize)
     o->pixels->video.resize(o->pixels->video.data, eo_obj,
                             &o->pixels->video,
                             obj->cur->cache.clip.w,
                             obj->cur->cache.clip.h);

   if (o->delayed.video_show)
     o->pixels->video.show(o->pixels->video.data, eo_obj, &o->pixels->video);
   else if (o->delayed.video_hide)
     o->pixels->video.hide(o->pixels->video.data, eo_obj, &o->pixels->video);

   o->delayed.video_move = EINA_FALSE;
   o->delayed.video_resize = EINA_FALSE;
   o->delayed.video_show = EINA_FALSE;
   o->delayed.video_hide = EINA_FALSE;
}

void *
_evas_object_image_surface_get(Evas_Object *eo, Evas_Object_Protected_Data *obj)
{
   Evas_Image_Data *pd = eo_data_scope_get(eo, MY_CLASS);

   if (pd->engine_data &&
       (pd->cur->image.w == obj->cur->geometry.w) &&
       (pd->cur->image.h == obj->cur->geometry.h))
     return pd->engine_data;

   if (pd->engine_data)
     ENFN->image_free(ENDT, pd->engine_data);

   // FIXME: alpha forced to 1 for now, need to figure out Evas alpha here
   EINA_COW_IMAGE_STATE_WRITE_BEGIN(pd, state_write)
     {
        pd->engine_data = ENFN->image_map_surface_new(ENDT,
                                                      obj->cur->geometry.w,
                                                      obj->cur->geometry.h,
                                                      1);
        state_write->image.w = obj->cur->geometry.w;
        state_write->image.h = obj->cur->geometry.h;
     }
   EINA_COW_IMAGE_STATE_WRITE_END(pd, state_write);

   return pd->engine_data;
}

#include "canvas/efl_canvas_image_internal.eo.c"

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
