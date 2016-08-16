#include "evas_filter_private.h"
#include "evas_blend_private.h"
#include "draw.h"

/* Apply geometrical transformations to a buffer.
 *
 * This filter is a very simplistic at the moment, future improvements require
 * more options to the API.
 */

static Eina_Bool
_vflip_cpu(Evas_Filter_Command *cmd)
{
   unsigned int src_len, src_stride, dst_len, dst_stride;
   uint8_t *in, *out = NULL;
   int w, h, sy, dy, oy, center, t, b, objh;
   Efl_Gfx_Colorspace cspace = cmd->output->alpha_only ? E_ALPHA : E_ARGB;
   int s0, s1, d0, d1;
   Eina_Bool ret = 0;

   if (!cmd->draw.A && (cmd->draw.rop == EFL_GFX_RENDER_OP_BLEND))
     return EINA_TRUE;

   w = cmd->input->w;
   h = cmd->input->h;
   in = _buffer_map_all(cmd->input->buffer, &src_len, E_READ, cspace, &src_stride);
   out = _buffer_map_all(cmd->output->buffer, &dst_len,
                         E_WRITE | ECTOR_BUFFER_ACCESS_FLAG_COW,
                         cspace, &dst_stride);

   EINA_SAFETY_ON_FALSE_GOTO(cmd->output->w == w, end);
   EINA_SAFETY_ON_FALSE_GOTO(cmd->output->h == h, end);
   EINA_SAFETY_ON_FALSE_GOTO(src_stride <= dst_stride, end);
   EINA_SAFETY_ON_NULL_GOTO(in, end);
   EINA_SAFETY_ON_NULL_GOTO(out, end);
   EINA_SAFETY_ON_FALSE_GOTO(in != out, end);

   oy = cmd->draw.oy;
   t = cmd->ctx->padt;
   b = cmd->ctx->padb;
   objh = h - t - b;
   center = t + objh / 2 + oy;

   if (oy >= 0)
     {
        s1 = d0 = center + (objh / 2) + oy;
        s0 = d1 = center - (objh / 2) - oy;
     }
   else
     {
        s1 = d0 = center + (objh / 2) - oy;
        s0 = d1 = center - (objh / 2) + oy;
     }

   /* avoid crashes */
   d0 = CLAMP(0, d0, h - 1);
   d1 = CLAMP(0, d1, h - 1);
   s0 = CLAMP(0, s0, h - 1);
   s1 = CLAMP(0, s1, h - 1);

   if (cmd->input->buffer == cmd->output->buffer)
     {
        /* flip a single buffer --> override its own contents */
        for (sy = s0, dy = d0; (dy >= d1) && (sy <= s1); sy++, dy--)
          {
             uint8_t* src = in + src_stride * sy;
             uint8_t* dst = out + dst_stride * dy;

             memcpy(dst, src, src_stride);
          }
     }
   else if (cspace == E_ALPHA)
     {
        /* blend onto a target (alpha -> alpha) */
        Alpha_Gfx_Func func = efl_draw_alpha_func_get(cmd->draw.rop, EINA_FALSE);
        EINA_SAFETY_ON_NULL_GOTO(func, end);

        for (sy = s0, dy = d0; (dy >= d1) && (sy <= s1); sy++, dy--)
          {
             uint8_t* src = in + src_stride * sy;
             uint8_t* dst = out + dst_stride * dy;

             func(src, dst, w);
          }
     }
   else
     {
        /* blend onto a target (rgba -> rgba) */
        uint32_t color = ARGB_JOIN(cmd->draw.A, cmd->draw.R, cmd->draw.G, cmd->draw.B);
        RGBA_Gfx_Func func;

        if (color == 0xFFFFFFFF)
          func = evas_common_gfx_func_composite_pixel_span_get(1, 0, 1, 1, _gfx_to_evas_render_op(cmd->draw.rop));
        else
          func = evas_common_gfx_func_composite_pixel_color_span_get(1, 0, color, 1, 1, _gfx_to_evas_render_op(cmd->draw.rop));
        EINA_SAFETY_ON_NULL_GOTO(func, end);

        for (sy = s0, dy = d0; (dy >= d1) && (sy <= s1); sy++, dy--)
          {
             uint32_t* src = (uint32_t *) (in + src_stride * sy);
             uint32_t* dst = (uint32_t *) (out + dst_stride * dy);

             func(src, NULL, color, dst, w);
          }
     }

   /* fill out outer areas */
   if (cmd->draw.rop == EFL_GFX_RENDER_OP_COPY)
     {
        if (d1 > 0)
          memset(out, 0, dst_stride * d1);
        if (d0 < (h - 1))
          memset(out + dst_stride * d0, 0, dst_stride * (h - d0 - 1));
     }

   ret = EINA_TRUE;

end:
   ector_buffer_unmap(cmd->input->buffer, in, src_len);
   ector_buffer_unmap(cmd->output->buffer, out, dst_len);
   return ret;
}

Evas_Filter_Apply_Func
evas_filter_transform_cpu_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);

   switch (cmd->transform.flags)
     {
      case EVAS_FILTER_TRANSFORM_VFLIP:
        return _vflip_cpu;
      default:
        CRI("Unknown transform flag %d", (int) cmd->transform.flags);
        return NULL;
     }
}
