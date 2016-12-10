#ifdef BUILD_MMX

static inline void
_box_blur_rgba_horiz_step_mmx(const DATA32* restrict const srcdata,
                              DATA32* restrict const dstdata,
                              const int* restrict const radii,
                              const int len,
                              const int loops)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_horiz_step(srcdata, dstdata, radii, len, loops);
}

static inline void
_box_blur_rgba_vert_step_mmx(const DATA32* restrict const srcdata,
                             DATA32* restrict const dstdata,
                             const int* restrict const radii,
                             const int len,
                             const int loops)
{
   // TODO: implement optimized code here and remove the following line:
   _box_blur_rgba_vert_step(srcdata, dstdata, radii, len, loops);
}

#endif
