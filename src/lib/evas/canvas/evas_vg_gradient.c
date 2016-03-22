#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"

#include <strings.h>

static void
_efl_vg_gradient_efl_gfx_gradient_base_stop_set(Eo *obj EINA_UNUSED,
                                                Efl_VG_Gradient_Data *pd,
                                                const Efl_Gfx_Gradient_Stop *colors,
                                                unsigned int length)
{
   pd->colors = realloc(pd->colors, length * sizeof(Efl_Gfx_Gradient_Stop));
   if (!pd->colors)
     {
        pd->colors_count = 0;
        return ;
     }

   memcpy(pd->colors, colors, length * sizeof(Efl_Gfx_Gradient_Stop));
   pd->colors_count = length;

   _efl_vg_base_changed(obj);
}

static void
_efl_vg_gradient_efl_gfx_gradient_base_stop_get(Eo *obj EINA_UNUSED,
                                                Efl_VG_Gradient_Data *pd,
                                                const Efl_Gfx_Gradient_Stop **colors,
                                                unsigned int *length)
{
   if (colors) *colors = pd->colors;
   if (length) *length = pd->colors_count;
}

static void
_efl_vg_gradient_efl_gfx_gradient_base_spread_set(Eo *obj EINA_UNUSED,
                                                  Efl_VG_Gradient_Data *pd,
                                                  Efl_Gfx_Gradient_Spread s)
{
   pd->s = s;

   _efl_vg_base_changed(obj);
}

static Efl_Gfx_Gradient_Spread
_efl_vg_gradient_efl_gfx_gradient_base_spread_get(Eo *obj EINA_UNUSED,
                                                  Efl_VG_Gradient_Data *pd)
{
   return pd->s;
}

static Eina_Bool
_efl_vg_gradient_efl_vg_base_interpolate(Eo *obj,
                                         Efl_VG_Gradient_Data *pd,
                                         const Efl_VG_Base *from, const Efl_VG_Base *to,
                                         double pos_map)
{
   Efl_VG_Gradient_Data *fromd, *tod;
   Efl_Gfx_Gradient_Stop *colors;
   unsigned int i;
   double from_map;
   Eina_Bool r;

   eo_do_super(obj, EFL_VG_GRADIENT_CLASS, r = efl_vg_interpolate(from, to, pos_map));

   fromd = eo_data_scope_get(from, EFL_VG_GRADIENT_CLASS);
   tod = eo_data_scope_get(to, EFL_VG_GRADIENT_CLASS);
   from_map = 1.0 - pos_map;

   if (!r) return EINA_FALSE;
   if (fromd->colors_count != tod->colors_count) return EINA_FALSE;

   colors = realloc(pd->colors, sizeof (Efl_Gfx_Gradient_Stop) * tod->colors_count);
   if (!colors) return EINA_FALSE;

   pd->colors = colors;

#define INTP(Pd, From, To, I, Member, From_Map, Pos_Map)                \
   Pd->colors[I].Member = From->colors[I].Member * From_Map + To->colors[I].Member * Pos_Map

   for (i = 0; i < fromd->colors_count; i++)
     {
        INTP(pd, fromd, tod, i, offset, from_map, pos_map);
        INTP(pd, fromd, tod, i, r, from_map, pos_map);
        INTP(pd, fromd, tod, i, g, from_map, pos_map);
        INTP(pd, fromd, tod, i, b, from_map, pos_map);
        INTP(pd, fromd, tod, i, a, from_map, pos_map);
     }

#undef INTP

   return EINA_TRUE;
}

static void
_efl_vg_gradient_efl_vg_base_dup(Eo *obj,
                                 Efl_VG_Gradient_Data *pd EINA_UNUSED,
                                 const Efl_VG_Base *from)
{
   Efl_VG_Gradient_Data *fromd;

   eo_do_super(obj, EFL_VG_GRADIENT_CLASS, efl_vg_dup(from));

   fromd = eo_data_scope_get(from, EFL_VG_GRADIENT_CLASS);

   eo_do(obj,
         efl_gfx_gradient_stop_set(fromd->colors, fromd->colors_count),
         efl_gfx_gradient_spread_set(fromd->s));
}

EAPI void
evas_vg_gradient_stop_set(Eo *obj, const Efl_Gfx_Gradient_Stop *colors, unsigned int length)
{
   eo_do(obj, efl_gfx_gradient_stop_set(colors, length));
}

EAPI void
evas_vg_gradient_stop_get(Eo *obj, const Efl_Gfx_Gradient_Stop **colors, unsigned int *length)
{
   eo_do(obj, efl_gfx_gradient_stop_get(colors, length));
}

EAPI void
evas_vg_gradient_spread_set(Eo *obj, Efl_Gfx_Gradient_Spread s)
{
   eo_do(obj, efl_gfx_gradient_spread_set(s));
}

EAPI Efl_Gfx_Gradient_Spread
evas_vg_gradient_spread_get(Eo *obj)
{
   Efl_Gfx_Gradient_Spread ret;

   return eo_do_ret(obj, ret, efl_gfx_gradient_spread_get());
}

#include "efl_vg_gradient.eo.c"
