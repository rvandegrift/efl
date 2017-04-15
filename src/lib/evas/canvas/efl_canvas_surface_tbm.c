#include "efl_canvas_surface.h"

#define MY_CLASS EFL_CANVAS_SURFACE_TBM_CLASS

typedef struct _Efl_Canvas_Surface_Tbm_Data
{
   Efl_Canvas_Surface_Data *base;
} Efl_Canvas_Surface_Tbm_Data;

EOLIAN static Eo *
_efl_canvas_surface_tbm_efl_object_constructor(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd)
{
   Evas_Object_Protected_Data *obj;

   eo = efl_constructor(efl_super(eo, MY_CLASS));
   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);
   if (!obj) return NULL;

   if (!ENFN->image_native_init(ENDT, EVAS_NATIVE_SURFACE_TBM))
     {
        ERR("TBM is not supported on this platform");
        return NULL;
     }

   pd->base = efl_data_ref(eo, EFL_CANVAS_SURFACE_MIXIN);
   pd->base->surf.type = EVAS_NATIVE_SURFACE_TBM;
   return eo;
}

EOLIAN static void
_efl_canvas_surface_tbm_efl_object_destructor(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd)
{
   Evas_Object_Protected_Data *obj;

   obj = efl_data_scope_get(eo, EFL_CANVAS_OBJECT_CLASS);

   ENFN->image_native_shutdown(ENDT, EVAS_NATIVE_SURFACE_TBM);
   efl_data_unref(eo, pd->base);
   efl_destructor(eo);
}

EOLIAN static Eina_Bool
_efl_canvas_surface_tbm_efl_canvas_surface_native_buffer_set(Eo *eo, Efl_Canvas_Surface_Tbm_Data *pd, void *buffer)
{
   pd->base->surf.data.tbm.buffer = buffer;
   if (!_evas_image_native_surface_set(eo, &pd->base->surf))
     {
        ERR("failed to set native buffer");
        pd->base->buffer = NULL;
        return EINA_FALSE;
     }
   pd->base->buffer = buffer;
   return EINA_TRUE;
}

#include "efl_canvas_surface_tbm.eo.c"
