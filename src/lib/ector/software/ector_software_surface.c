#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Ector.h>
#include <software/Ector_Software.h>

#include "ector_private.h"
#include "ector_software_private.h"

#define MY_CLASS ECTOR_SOFTWARE_SURFACE_CLASS

static Ector_Renderer *
_ector_software_surface_ector_generic_surface_renderer_factory_new(Eo *obj,
                                                                   Ector_Software_Surface_Data *pd EINA_UNUSED,
                                                                   const Eo_Class *type)
{
   if (type == ECTOR_RENDERER_GENERIC_SHAPE_MIXIN)
     return eo_add(ECTOR_RENDERER_SOFTWARE_SHAPE_CLASS, NULL,
                   ector_renderer_surface_set(obj));
   else if (type == ECTOR_RENDERER_GENERIC_GRADIENT_LINEAR_MIXIN)
     return eo_add(ECTOR_RENDERER_SOFTWARE_GRADIENT_LINEAR_CLASS, NULL,
                   ector_renderer_surface_set(obj));
   else if (type == ECTOR_RENDERER_GENERIC_GRADIENT_RADIAL_MIXIN)
     return eo_add(ECTOR_RENDERER_SOFTWARE_GRADIENT_RADIAL_CLASS, NULL,
                   ector_renderer_surface_set(obj));
   else if (type == ECTOR_RENDERER_GENERIC_BUFFER_MIXIN)
     return eo_add(ECTOR_RENDERER_SOFTWARE_BUFFER_CLASS, NULL,
                   ector_renderer_surface_set(obj));
   ERR("Couldn't find class for type: %s\n", eo_class_name_get(type));
   return NULL;
}

static Eo *
_ector_software_surface_eo_base_constructor(Eo *obj, Ector_Software_Surface_Data *pd)
{
   obj = eo_do_super_ret(obj, MY_CLASS, obj, eo_constructor());
   pd->rasterizer = (Software_Rasterizer *) calloc(1, sizeof(Software_Rasterizer));
   ector_software_rasterizer_init(pd->rasterizer);
   pd->rasterizer->fill_data.raster_buffer = eo_data_ref(obj, ECTOR_SOFTWARE_BUFFER_BASE_MIXIN);
   return obj;
}

static void
_ector_software_surface_eo_base_destructor(Eo *obj, Ector_Software_Surface_Data *pd)
{
   ector_software_rasterizer_done(pd->rasterizer);
   eo_data_unref(obj, pd->rasterizer->fill_data.raster_buffer);
   free(pd->rasterizer);
   pd->rasterizer = NULL;
   eo_do_super(obj, ECTOR_SOFTWARE_SURFACE_CLASS, eo_destructor());
}

static void
_ector_software_surface_ector_generic_surface_reference_point_set(Eo *obj EINA_UNUSED,
                                                                  Ector_Software_Surface_Data *pd,
                                                                  int x, int y)
{
   pd->x = x;
   pd->y = y;
}

#include "ector_software_surface.eo.c"
#include "ector_renderer_software_base.eo.c"
