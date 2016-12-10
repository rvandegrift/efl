#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>
#include <cairo/Ector_Cairo.h>

#include "ector_private.h"
#include "ector_cairo_private.h"

static cairo_pattern_t *(*cairo_pattern_create_radial)(double cx0, double cy0,
                                                       double radius0,
                                                       double cx1, double cy1,
                                                       double radius1) = NULL;
static void (*cairo_set_source)(cairo_t *cr, cairo_pattern_t *source) = NULL;
static void (*cairo_fill)(cairo_t *cr) = NULL;
static void (*cairo_arc)(cairo_t *cr,
                         double xc, double yc,
                         double radius,
                         double angle1, double angle2) = NULL;
static void (*cairo_pattern_destroy)(cairo_pattern_t *pattern) = NULL;

static void (*cairo_pattern_set_extend)(cairo_pattern_t *pattern, cairo_extend_t extend) = NULL;

// FIXME: as long as it is not possible to directly access the parent structure
//  this will be duplicated from the linear gradient renderer
typedef struct _Ector_Renderer_Cairo_Gradient_Radial_Data Ector_Renderer_Cairo_Gradient_Radial_Data;
struct _Ector_Renderer_Cairo_Gradient_Radial_Data
{
   Ector_Cairo_Surface_Data *parent;
};

static Eina_Bool
_ector_renderer_cairo_gradient_radial_ector_renderer_prepare(Eo *obj, Ector_Renderer_Cairo_Gradient_Radial_Data *pd)
{
   ector_renderer_prepare(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS));

   if (!pd->parent)
     {
        Ector_Renderer_Data *base;

        base = eo_data_scope_get(obj, ECTOR_RENDERER_CLASS);
        pd->parent = eo_data_xref(base->surface, ECTOR_CAIRO_SURFACE_CLASS, obj);
     }

   return EINA_FALSE;
}

static cairo_pattern_t *
_ector_renderer_cairo_gradient_radial_prepare(Eo *obj EINA_UNUSED,
                                              Ector_Renderer_Gradient_Radial_Data *grd,
                                              Ector_Renderer_Gradient_Data *gd,
                                              unsigned int mul_col)
{
   cairo_pattern_t *pat;

   pat = cairo_pattern_create_radial(grd->focal.x, grd->focal.y, 0,
                                     grd->radial.x, grd->radial.y, grd->radius);
   if (!pat) return NULL;

   _ector_renderer_cairo_gradient_prepare(pat, gd, mul_col);

   cairo_pattern_set_extend(pat, _ector_cairo_extent_get(gd->s));

   return pat;
}

// Clearly duplicated and should be in a common place...
static Eina_Bool
_ector_renderer_cairo_gradient_radial_ector_renderer_draw(Eo *obj, Ector_Renderer_Cairo_Gradient_Radial_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips, unsigned int mul_col)
{
   Ector_Renderer_Gradient_Radial_Data *grd;
   Ector_Renderer_Gradient_Data *gd;
   cairo_pattern_t *pat;

   // FIXME: Handle clipping and generally make it work :-)
   grd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_MIXIN);
   if (!grd || !gd) return EINA_FALSE;

   pat = _ector_renderer_cairo_gradient_radial_prepare(obj, grd, gd, mul_col);
   if (!pat) return EINA_FALSE;

   ector_renderer_draw(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS), op, clips, mul_col);

   cairo_arc(pd->parent->cairo,
             grd->radial.x, grd->radial.y,
             grd->radius,
             0, 2 * M_PI);
   cairo_set_source(pd->parent->cairo, pat);
   cairo_fill(pd->parent->cairo);
   cairo_pattern_destroy(pat);

   return EINA_TRUE;
}

// Clearly duplicated and should be in a common place...
static Eina_Bool
_ector_renderer_cairo_gradient_radial_ector_renderer_cairo_fill(Eo *obj,
                                                                     Ector_Renderer_Cairo_Gradient_Radial_Data *pd,
                                                                     unsigned int mul_col)
{
   Ector_Renderer_Gradient_Radial_Data *grd;
   Ector_Renderer_Gradient_Data *gd;
   cairo_pattern_t *pat;

   grd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_MIXIN);
   if (!grd || !gd) return EINA_FALSE;

   pat = _ector_renderer_cairo_gradient_radial_prepare(obj, grd, gd, mul_col);
   if (!pat) return EINA_FALSE;

   cairo_set_source(pd->parent->cairo, pat);
   cairo_pattern_destroy(pat);

   return EINA_TRUE;
}

static void
_ector_renderer_cairo_gradient_radial_ector_renderer_bounds_get(Eo *obj EINA_UNUSED,
                                                                             Ector_Renderer_Cairo_Gradient_Radial_Data *pd EINA_UNUSED,
                                                                             Eina_Rectangle *r)
{
   Ector_Renderer_Gradient_Radial_Data *gld;
   Ector_Renderer_Cairo_Data *bd;

   gld = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN);
   bd = eo_data_scope_get(obj, ECTOR_RENDERER_CAIRO_CLASS);
   EINA_RECTANGLE_SET(r,
                      bd->generic->origin.x + gld->radial.x - gld->radius,
                      bd->generic->origin.y + gld->radial.y - gld->radius,
                      gld->radius * 2, gld->radius * 2);
}

static Eo_Base *
_ector_renderer_cairo_gradient_radial_eo_base_finalize(Eo *obj, Ector_Renderer_Cairo_Gradient_Radial_Data *pd EINA_UNUSED)
{
   Ector_Renderer_Data *base;

   obj = eo_finalize(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS));
   if (!obj) return NULL;

   base = eo_data_scope_get(obj, ECTOR_RENDERER_CLASS);

   USE(base, cairo_set_source, NULL);
   USE(base, cairo_pattern_destroy, NULL);
   USE(base, cairo_arc, NULL);
   USE(base, cairo_fill, NULL);
   USE(base, cairo_set_source, NULL);
   USE(base, cairo_pattern_destroy, NULL);
   USE(base, cairo_pattern_set_extend, NULL);
   USE(base, cairo_pattern_create_radial, NULL);

   return obj;
}

static void
_ector_renderer_cairo_gradient_radial_eo_base_destructor(Eo *obj,
                                                         Ector_Renderer_Cairo_Gradient_Radial_Data *pd)
{
   Ector_Renderer_Data *base;

   base = eo_data_scope_get(obj, ECTOR_RENDERER_CLASS);
   eo_data_xunref(base->surface, pd->parent, obj);

   eo_destructor(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS));
}

static void
_ector_renderer_cairo_gradient_radial_efl_gfx_gradient_stop_set(Eo *obj,
                                                                     Ector_Renderer_Cairo_Gradient_Radial_Data *pd EINA_UNUSED,
                                                                     const Efl_Gfx_Gradient_Stop *colors,
                                                                     unsigned int length)
{
   efl_gfx_gradient_stop_set(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS), colors, length);
}

static unsigned int
_ector_renderer_cairo_gradient_radial_ector_renderer_crc_get(Eo *obj, Ector_Renderer_Cairo_Gradient_Radial_Data *pd EINA_UNUSED)
{
   Ector_Renderer_Gradient_Radial_Data *grd;
   Ector_Renderer_Gradient_Data *gd;
   unsigned int crc;

   crc = ector_renderer_crc_get(eo_super(obj, ECTOR_RENDERER_CAIRO_GRADIENT_RADIAL_CLASS));

   grd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_RADIAL_MIXIN);
   gd = eo_data_scope_get(obj, ECTOR_RENDERER_GRADIENT_MIXIN);
   if (!grd || !gd) return crc;

   crc = eina_crc((void*) gd->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (gd->colors_count)
     crc = eina_crc((void*) gd->colors, sizeof (Efl_Gfx_Gradient_Stop) * gd->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) grd, sizeof (Ector_Renderer_Gradient_Radial_Data), crc, EINA_FALSE);

   return crc;
}

#include "ector_renderer_cairo_gradient_radial.eo.c"
