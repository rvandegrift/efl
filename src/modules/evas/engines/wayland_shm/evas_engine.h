/* Portions of this code have been derived from Weston
 *
 * Copyright © 2008-2012 Kristian Høgsberg
 * Copyright © 2010-2012 Intel Corporation
 * Copyright © 2010-2011 Benjamin Franzke
 * Copyright © 2011-2012 Collabora, Ltd.
 * Copyright © 2010 Red Hat <mjg@redhat.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _EVAS_ENGINE_H
# define _EVAS_ENGINE_H

//# define LOGFNS 1

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN(fl, ln, fn) printf("-EVAS-WL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define LOGFN(fl, ln, fn)
# endif

extern int _evas_engine_way_shm_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_way_shm_log_dom, __VA_ARGS__)

# include <wayland-client.h>
# include "../software_generic/Evas_Engine_Software_Generic.h"
# include "Evas_Engine_Wayland_Shm.h"

# define MAX_BUFFERS 4

typedef struct _Shm_Pool Shm_Pool;
struct _Shm_Pool
{
   struct wl_shm_pool *pool;
   size_t size, used;
   void *data;
};

typedef struct _Shm_Data Shm_Data;
struct _Shm_Data
{
   struct wl_buffer *buffer;
   Shm_Pool *pool;
   void *map;
};

typedef struct _Shm_Leaf Shm_Leaf;
struct _Shm_Leaf
{
   int w, h, busy, age;
   Shm_Data *data;
   Shm_Pool *resize_pool;
   Eina_Bool valid : 1;
   Eina_Bool reconfigure : 1;
   Eina_Bool drawn : 1;
};

typedef struct _Shm_Surface Shm_Surface;
struct _Shm_Surface
{
   struct wl_display *disp;
   struct wl_shm *shm;
   struct wl_surface *surface;
   int w, h;
   int dx, dy;
   int num_buff;
   int compositor_version;

   Shm_Leaf leaf[MAX_BUFFERS];
   Shm_Leaf *current;

   Eina_Bool alpha : 1;
};

struct _Outbuf
{
   int w, h;
   int rotation;
   int onebuf;
   int num_buff;
   Outbuf_Depth depth;

   Evas_Engine_Info_Wayland_Shm *info;

   Shm_Surface *surface;

   struct 
     {
        /* one big buffer for updates. flushed on idle_flush */
        RGBA_Image *onebuf;
        Eina_Array onebuf_regions;

        /* a list of pending regions to write out */
        Eina_List *pending_writes;

        /* list of previous frame pending regions to write out */
        Eina_List *prev_pending_writes;

        /* Eina_Bool redraw : 1; */
        Eina_Bool destination_alpha : 1;
     } priv;
};

Shm_Surface *_evas_shm_surface_create(struct wl_display *disp, struct wl_shm *shm, struct wl_surface *surface, int w, int h, int num_buff, Eina_Bool alpha, int compositor_version);
void _evas_shm_surface_destroy(Shm_Surface *surface);
void _evas_shm_surface_reconfigure(Shm_Surface *surface, int dx, int dy, int w, int h, int num_buff, uint32_t flags);
void *_evas_shm_surface_data_get(Shm_Surface *surface, int *w, int *h);
int _evas_shm_surface_assign(Shm_Surface *surface);
void _evas_shm_surface_post(Shm_Surface *surface, Eina_Rectangle *rects, unsigned int count);

Outbuf *_evas_outbuf_setup(int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, struct wl_shm *shm, struct wl_surface *surface, struct wl_display *disp, int compositor_version);
void _evas_outbuf_free(Outbuf *ob);
void _evas_outbuf_flush(Outbuf *ob, Tilebuf_Rect *rects, Evas_Render_Mode render_mode);
void _evas_outbuf_idle_flush(Outbuf *ob);

Render_Engine_Swap_Mode _evas_outbuf_swap_mode_get(Outbuf *ob);
int _evas_outbuf_rotation_get(Outbuf *ob);
void _evas_outbuf_reconfigure(Outbuf *ob, int x, int y, int w, int h, int rot, Outbuf_Depth depth, Eina_Bool alpha, Eina_Bool resize);
void *_evas_outbuf_update_region_new(Outbuf *ob, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch);
void _evas_outbuf_update_region_push(Outbuf *ob, RGBA_Image *update, int x, int y, int w, int h);
void _evas_outbuf_update_region_free(Outbuf *ob, RGBA_Image *update);

#endif
