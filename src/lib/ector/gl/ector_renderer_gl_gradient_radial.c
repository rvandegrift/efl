#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ector.h>

#include "gl/Ector_GL.h"
#include "ector_private.h"
#include "ector_gl_private.h"

typedef struct _Ector_Renderer_GL_Gradient_Radial_Data Ector_Renderer_GL_Gradient_Radial_Data;
struct _Ector_Renderer_GL_Gradient_Radial_Data
{
   Ector_Renderer_Generic_Gradient_Radial_Data *radial;
   Ector_Renderer_Generic_Gradient_Data *gradient;
   Ector_Renderer_Generic_Base_Data *base;
};

static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_generic_base_prepare(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   // FIXME: prepare something
   (void) obj;
   (void) pd;

   return EINA_TRUE;
}

static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_generic_base_draw(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd, Efl_Gfx_Render_Op op, Eina_Array *clips, unsigned int mul_col)
{
   eo_do_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS,
               ector_renderer_draw(op, clips, mul_col));

   // FIXME: draw something !
   (void) pd;

   return EINA_TRUE;
}

static Eina_Bool
_ector_renderer_gl_gradient_radial_ector_renderer_gl_base_fill(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd, uint64_t flags, GLshort *vertex, unsigned int vertex_count, unsigned int mul_col)
{
   // FIXME: The idea here is to select the right shader and push the needed parameter for it
   // along with the other value
   (void) obj;
   (void) pd;
   (void) flags;
   (void) vertex;
   (void) vertex_count;
   (void) mul_col;

   return EINA_TRUE;
}

static void
_ector_renderer_gl_gradient_radial_ector_renderer_generic_base_bounds_get(Eo *obj EINA_UNUSED,
                                                                          Ector_Renderer_GL_Gradient_Radial_Data *pd,
                                                                          Eina_Rectangle *r)
{
   EINA_RECTANGLE_SET(r,
                      pd->base->origin.x + pd->radial->radial.x - pd->radial->radius,
                      pd->base->origin.y + pd->radial->radial.y - pd->radial->radius,
                      pd->radial->radius * 2, pd->radial->radius * 2 );
}

static unsigned int
_ector_renderer_gl_gradient_radial_ector_renderer_generic_base_crc_get(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   unsigned int crc;

   eo_do_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS,
               crc = ector_renderer_crc_get());

   crc = eina_crc((void*) pd->gradient->s, sizeof (Efl_Gfx_Gradient_Spread), crc, EINA_FALSE);
   if (pd->gradient->colors_count)
     crc = eina_crc((void*) pd->gradient->colors, sizeof (Efl_Gfx_Gradient_Stop) * pd->gradient->colors_count, crc, EINA_FALSE);
   crc = eina_crc((void*) pd->radial, sizeof (Ector_Renderer_Generic_Gradient_Radial_Data), crc, EINA_FALSE);

   return crc;
}

static Eo_Base *
_ector_renderer_gl_gradient_radial_eo_base_constructor(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   eo_do_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS, obj = eo_constructor());

   if (!obj) return NULL;

   pd->base = eo_data_xref(obj, ECTOR_RENDERER_GENERIC_BASE_CLASS, obj);
   pd->radial = eo_data_xref(obj, ECTOR_RENDERER_GENERIC_GRADIENT_RADIAL_MIXIN, obj);
   pd->gradient = eo_data_xref(obj, ECTOR_RENDERER_GENERIC_GRADIENT_MIXIN, obj);

   return obj;
}

static void
_ector_renderer_gl_gradient_radial_eo_base_destructor(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd)
{
   eo_data_xunref(obj, pd->base, obj);
   eo_data_xunref(obj, pd->radial, obj);
   eo_data_xunref(obj, pd->gradient, obj);
}

static void
_ector_renderer_gl_gradient_radial_efl_gfx_gradient_base_stop_set(Eo *obj, Ector_Renderer_GL_Gradient_Radial_Data *pd EINA_UNUSED, const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   eo_do_super(obj, ECTOR_RENDERER_GL_GRADIENT_RADIAL_CLASS,
               efl_gfx_gradient_stop_set(colors, length));
}

#include "ector_renderer_gl_gradient_radial.eo.c"
