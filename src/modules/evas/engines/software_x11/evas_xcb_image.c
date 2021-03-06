#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "evas_common_private.h"
#include "evas_xcb_image.h"

static void 
_evas_xcb_image_update(void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n;

   n = im->native.data;

   if (ecore_x_image_get(n->ns_data.x11.exim, n->ns_data.x11.pixmap, 0, 0, x, y, w, h))
     {
        char *pix;
        int bpl, rows, bpp;

        pix = ecore_x_image_data_get(n->ns_data.x11.exim, &bpl, &rows, &bpp);
        if (!ecore_x_image_is_argb32_get(n->ns_data.x11.exim))
          {
             Ecore_X_Colormap colormap;

             if (!im->image.data)
               im->image.data = (DATA32 *)malloc(im->cache_entry.w * im->cache_entry.h * sizeof(DATA32));
             colormap = ecore_x_default_colormap_get(ecore_x_display_get(), ecore_x_default_screen_get());
             ecore_x_image_to_argb_convert(pix, bpp, bpl, colormap, n->ns_data.x11.visual,
                                           x, y, w, h, im->image.data, 
                                           (w * sizeof(int)), 0, 0);
          }
        else
          im->image.data = (DATA32 *)pix;
     }
}

static void 
_native_cb_bind(void *image, int x, int y, int w, int h)
{
   RGBA_Image *im = image;
   Native *n;

   n = im->native.data;

   if ((n) && (n->ns.type == EVAS_NATIVE_SURFACE_X11))
     _evas_xcb_image_update(image, x, y, w, h);
}

static void 
_native_cb_free(void *image)
{
   RGBA_Image *im;
   Native *n;

   im = image;
   n = im->native.data;

   if (n->ns_data.x11.exim)
     {
        ecore_x_image_free(n->ns_data.x11.exim);
        n->ns_data.x11.exim = NULL;
     }
   n->ns_data.x11.visual = NULL;

   im->native.data = NULL;
   im->native.func.bind = NULL;
   im->native.func.unbind = NULL;
   im->native.func.free = NULL;
   im->image.data = NULL;

   free(n);
}

void *
evas_xcb_image_native_set(void *data EINA_UNUSED, void *image, void *native)
{
   RGBA_Image *im;
   Evas_Native_Surface *ns;

   im = image;
   ns = native;
   if ((ns) && (ns->type == EVAS_NATIVE_SURFACE_X11))
     {
        Native *n = NULL;
        Ecore_X_Image *exim = NULL;
        Ecore_X_Visual *vis = NULL;
        Ecore_X_Pixmap pm = 0;
        int w, h, depth;

        vis = ns->data.x11.visual;
        pm = ns->data.x11.pixmap;

        depth = ecore_x_drawable_depth_get(pm);
        w = im->cache_entry.w;
        h = im->cache_entry.h;

        n = calloc(1, sizeof(Native));
        if (!n) return NULL;

        exim = ecore_x_image_new(w, h, vis, depth);
        if (!exim)
          {
             ERR("Failed to create new Ecore_X_Image");
             free(n);
             return NULL;
          }

        memcpy(&(n->ns), ns, sizeof(Evas_Native_Surface));
        n->ns_data.x11.pixmap = pm;
        n->ns_data.x11.visual = vis;
        n->ns_data.x11.exim = exim;

        im->native.data = n;
        im->native.func.bind = _native_cb_bind;
        im->native.func.unbind = NULL;
        im->native.func.free = _native_cb_free;

        _evas_xcb_image_update(image, 0, 0, w, h);
     }

   return im;
}
