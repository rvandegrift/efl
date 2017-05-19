#include "ecore_drm2_private.h"

static Eina_Bool
_fb2_create(Ecore_Drm2_Fb *fb)
{
   drm_mode_fb_cmd2 cmd;
   uint32_t hdls[4] = { 0 }, pitches[4] = { 0 }, offsets[4] = { 0 };
   uint64_t modifiers[4] = { 0 };

   hdls[0] = fb->hdl;
   pitches[0] = fb->stride;
   offsets[0] = 0;
   modifiers[0] = 0;

   memset(&cmd, 0, sizeof(drm_mode_fb_cmd2));
   cmd.fb_id = 0;
   cmd.width = fb->w;
   cmd.height = fb->h;
   cmd.pixel_format = fb->format;
   cmd.flags = 0;
   memcpy(cmd.handles, hdls, 4 * sizeof(hdls[0]));
   memcpy(cmd.pitches, pitches, 4 * sizeof(pitches[0]));
   memcpy(cmd.offsets, offsets, 4 * sizeof(offsets[0]));
   memcpy(cmd.modifier, modifiers, 4 * sizeof(modifiers[0]));

   if (sym_drmIoctl(fb->fd, DRM_IOCTL_MODE_ADDFB2, &cmd))
     return EINA_FALSE;

   fb->id = cmd.fb_id;

   return EINA_TRUE;
}

#ifdef HAVE_ATOMIC_DRM
static int
_fb_atomic_flip(Ecore_Drm2_Output *output, Ecore_Drm2_Plane_State *pstate, uint32_t flags)
{
   int ret = 0;
   drmModeAtomicReq *req = NULL;
   Ecore_Drm2_Crtc_State *cstate;

   req = sym_drmModeAtomicAlloc();
   if (!req) return -1;

   sym_drmModeAtomicSetCursor(req, 0);

   cstate = output->crtc_state;

   ret = sym_drmModeAtomicAddProperty(req, cstate->obj_id, cstate->mode.id,
                                      cstate->mode.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, cstate->obj_id, cstate->active.id,
                                      cstate->active.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->cid.id, pstate->cid.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->fid.id, pstate->fid.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->sx.id, pstate->sx.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->sy.id, pstate->sy.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->sw.id, pstate->sw.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->sh.id, pstate->sh.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->cx.id, pstate->cx.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->cy.id, pstate->cy.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->cw.id, pstate->cw.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicAddProperty(req, pstate->obj_id,
                                      pstate->ch.id, pstate->ch.value);
   if (ret < 0) goto err;

   ret = sym_drmModeAtomicCommit(output->fd, req, flags, output->user_data);
   if (ret < 0) ERR("Failed to commit Atomic FB Flip: %m");
   else ret = 0;

err:
   sym_drmModeAtomicFree(req);
   return ret;
}
#endif

EAPI Ecore_Drm2_Fb *
ecore_drm2_fb_create(int fd, int width, int height, int depth, int bpp, unsigned int format)
{
   Ecore_Drm2_Fb *fb;
   drm_mode_create_dumb carg;
   drm_mode_destroy_dumb darg;
   drm_mode_map_dumb marg;
   int ret;

   EINA_SAFETY_ON_TRUE_RETURN_VAL((fd < 0), NULL);

   fb = calloc(1, sizeof(Ecore_Drm2_Fb));
   if (!fb) return NULL;

   fb->fd = fd;
   fb->w = width;
   fb->h = height;
   fb->bpp = bpp;
   fb->depth = depth;
   fb->format = format;

   memset(&carg, 0, sizeof(drm_mode_create_dumb));
   carg.bpp = bpp;
   carg.width = width;
   carg.height = height;

   ret = sym_drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &carg);
   if (ret) goto err;

   fb->hdl = carg.handle;
   fb->size = carg.size;
   fb->stride = carg.pitch;

   if (!_fb2_create(fb))
     {
        ret =
          sym_drmModeAddFB(fd, width, height, depth, bpp,
                           fb->stride, fb->hdl, &fb->id);
        if (ret)
          {
             ERR("Could not add framebuffer: %m");
             goto add_err;
          }
     }

   memset(&marg, 0, sizeof(drm_mode_map_dumb));
   marg.handle = fb->hdl;
   ret = sym_drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &marg);
   if (ret)
     {
        ERR("Could not map framebuffer: %m");
        goto map_err;
     }

   fb->mmap = mmap(NULL, fb->size, PROT_WRITE, MAP_SHARED, fd, marg.offset);
   if (fb->mmap == MAP_FAILED)
     {
        ERR("Could not mmap framebuffer memory: %m");
        goto map_err;
     }

   return fb;

map_err:
   sym_drmModeRmFB(fd, fb->id);
add_err:
   memset(&darg, 0, sizeof(drm_mode_destroy_dumb));
   darg.handle = fb->hdl;
   sym_drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
err:
   free(fb);
   return NULL;
}

EAPI Ecore_Drm2_Fb *
ecore_drm2_fb_gbm_create(int fd, int width, int height, int depth, int bpp, unsigned int format, unsigned int handle, unsigned int stride, void *bo)
{
   drm_mode_map_dumb marg;
   Ecore_Drm2_Fb *fb;
   int ret;

   EINA_SAFETY_ON_TRUE_RETURN_VAL((fd < 0), NULL);

   fb = calloc(1, sizeof(Ecore_Drm2_Fb));
   if (!fb) return NULL;

   fb->gbm = EINA_TRUE;
   fb->gbm_bo = bo;

   fb->fd = fd;
   fb->w = width;
   fb->h = height;
   fb->bpp = bpp;
   fb->depth = depth;
   fb->format = format;
   fb->stride = stride;
   fb->size = fb->stride * fb->h;
   fb->hdl = handle;

   if (!_fb2_create(fb))
     {
        if (sym_drmModeAddFB(fd, width, height, depth, bpp,
                             fb->stride, fb->hdl, &fb->id))
          {
             ERR("Could not add framebuffer: %m");
             goto err;
          }
     }

   /* mmap it if we can so screenshots are easy */
   memset(&marg, 0, sizeof(drm_mode_map_dumb));
   marg.handle = fb->hdl;
   ret = sym_drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &marg);
   if (!ret)
     {
        fb->mmap = mmap(NULL, fb->size, PROT_WRITE, MAP_SHARED, fd, marg.offset);
        if (fb->mmap == MAP_FAILED) fb->mmap = NULL;
     }
   return fb;

err:
   free(fb);
   return NULL;
}

EAPI void
ecore_drm2_fb_destroy(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);

   if (fb->mmap) munmap(fb->mmap, fb->size);

   if (fb->id) sym_drmModeRmFB(fb->fd, fb->id);

   if (!fb->gbm)
     {
        drm_mode_destroy_dumb darg;

        memset(&darg, 0, sizeof(drm_mode_destroy_dumb));
        darg.handle = fb->hdl;
        sym_drmIoctl(fb->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &darg);
     }

   free(fb);
}

EAPI void *
ecore_drm2_fb_data_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, NULL);
   return fb->mmap;
}

EAPI unsigned int
ecore_drm2_fb_size_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, 0);
   return fb->size;
}

EAPI unsigned int
ecore_drm2_fb_stride_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, 0);
   return fb->stride;
}

EAPI void
ecore_drm2_fb_dirty(Ecore_Drm2_Fb *fb, Eina_Rectangle *rects, unsigned int count)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);
   EINA_SAFETY_ON_NULL_RETURN(rects);

#ifdef DRM_MODE_FEATURE_DIRTYFB
   drmModeClip *clip;
   unsigned int i = 0;
   int ret;

   clip = alloca(count * sizeof(drmModeClip));
   for (i = 0; i < count; i++)
     {
        clip[i].x1 = rects[i].x;
        clip[i].y1 = rects[i].y;
        clip[i].x2 = rects[i].w;
        clip[i].y2 = rects[i].h;
     }

   ret = sym_drmModeDirtyFB(fb->fd, fb->id, clip, count);
   if ((ret) && (ret == -EINVAL))
     WRN("Could not mark framebuffer as dirty: %m");
#endif
}

static void
_release_buffer(Ecore_Drm2_Output *output, Ecore_Drm2_Fb *b)
{
   b->busy = EINA_FALSE;
   if (output->release_cb) output->release_cb(output->release_data, b);
}

EAPI Eina_Bool
ecore_drm2_fb_flip_complete(Ecore_Drm2_Output *output)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(output, EINA_FALSE);

   if (output->current && (output->current != output->pending))
     _release_buffer(output, output->current);
   output->current = output->pending;
   output->pending = NULL;

   return !!output->next;
}

EAPI int
ecore_drm2_fb_flip(Ecore_Drm2_Fb *fb, Ecore_Drm2_Output *output)
{
   int ret = 0;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output, -1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output->current_mode, -1);

   if (!output->enabled) return -1;

   if (output->pending)
     {
        if (output->next) _release_buffer(output, output->next);
        output->next = fb;
        if (output->next) output->next->busy = EINA_TRUE;
        return 0;
     }
   if (!fb) fb = output->next;

   /* So we can generate a tick by flipping to the current fb */
   if (!fb) fb = output->current;

   if (output->next)
     {
        output->next->busy = EINA_FALSE;
        output->next = NULL;
     }

   /* If we don't have an fb to set by now, BAIL! */
   if (!fb) return -1;

#ifdef HAVE_ATOMIC_DRM
   if (_ecore_drm2_use_atomic)
     {
        Ecore_Drm2_Plane_State *pstate;
        uint32_t flags =
          DRM_MODE_ATOMIC_NONBLOCK | DRM_MODE_PAGE_FLIP_EVENT |
          DRM_MODE_ATOMIC_ALLOW_MODESET;

        pstate = output->plane_state;

        pstate->cid.value = output->crtc_id;
        pstate->fid.value = fb->id;

        pstate->sx.value = 0;
        pstate->sy.value = 0;
        pstate->sw.value = fb->w << 16;
        pstate->sh.value = fb->h << 16;
        pstate->cx.value = output->x;
        pstate->cy.value = output->y;
        pstate->cw.value = output->current_mode->width;
        pstate->ch.value = output->current_mode->height;

        ret = _fb_atomic_flip(output, pstate, flags);
        if ((ret < 0) && (errno != EBUSY))
          {
             ERR("Atomic Pageflip Failed for Crtc %u on Connector %u: %m",
                 output->crtc_id, output->conn_id);
             return ret;
          }
        else if (ret < 0)
          {
             output->next = fb;
             if (output->next) output->next->busy = EINA_TRUE;

             return 0;
          }

        output->pending = fb;
        output->pending->busy = EINA_TRUE;

        return 0;
     }
   else
#endif
     {
        Eina_Bool repeat;
        int count = 0;

        if ((!output->current) ||
            (output->current->stride != fb->stride))
          {
             ret =
               sym_drmModeSetCrtc(fb->fd, output->crtc_id, fb->id,
                                  output->x, output->y, &output->conn_id, 1,
                                  &output->current_mode->info);
             if (ret)
               {
                  ERR("Failed to set Mode %dx%d for Output %s: %m",
                      output->current_mode->width, output->current_mode->height,
                      output->name);
                  return ret;
               }

             if (output->current) _release_buffer(output, output->current);
             output->current = fb;
             output->current->busy = EINA_TRUE;
             output->next = NULL;
             /* We used to return here, but now that the ticker is fixed this
              * can leave us hanging waiting for a tick to happen forever.
              * Instead, we now fall through the the flip path to make sure
              * even this first set can cause a flip callback.
              */
          }

        do
          {
             static Eina_Bool bugged_about_bug = EINA_FALSE;
             repeat = EINA_FALSE;
             ret = sym_drmModePageFlip(fb->fd, output->crtc_id, fb->id,
                                       DRM_MODE_PAGE_FLIP_EVENT,
                                       output->user_data);
             /* Some drivers (RPI - looking at you) are broken and produce
              * flip events before they are ready for another flip, so be
              * a little robust in the face of badness and try a few times
              * until we can flip or we give up (100 tries with a yield
              * between each try). We can't expect everyone to run the
              * latest bleeding edge kernel IF a workaround is possible
              * in userspace, so do this.
              * We only report this as an ERR once since if it will
              * generate a huge amount of spam otherwise. */
             if ((ret < 0) && (errno == EBUSY))
               {
                  repeat = EINA_TRUE;
                  if (count == 0 && !bugged_about_bug)
                    {
                       ERR("Pageflip fail - EBUSY from drmModePageFlip - "
                           "This is either a kernel bug or an EFL one.");
                       bugged_about_bug = EINA_TRUE;
                    }
                  count++;
                  if (count > 500)
                    {
                       ERR("Pageflip EBUSY for %i tries - give up", count);
                       break;
                    }
                  usleep(100);
               }
          }
        while (repeat);

        if ((ret == 0) && (count > 0))
          DBG("Pageflip finally succeeded after %i tries due to EBUSY", count);

        if ((ret < 0) && (errno != EBUSY))
          {
             ERR("Pageflip Failed for Crtc %u on Connector %u: %m",
                 output->crtc_id, output->conn_id);
             return ret;
          }
        else if (ret < 0)
          {
             output->next = fb;
             output->next->busy = EINA_TRUE;
             return 0;
          }

        output->pending = fb;
        output->pending->busy = EINA_TRUE;
        return 0;
     }
}

EAPI Eina_Bool
ecore_drm2_fb_busy_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, EINA_FALSE);
   return fb->busy;
}

EAPI void
ecore_drm2_fb_busy_set(Ecore_Drm2_Fb *fb, Eina_Bool busy)
{
   EINA_SAFETY_ON_NULL_RETURN(fb);
   fb->busy = busy;
}

EAPI Eina_Bool
ecore_drm2_fb_release(Ecore_Drm2_Output *o, Eina_Bool panic)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(o, EINA_FALSE);

   if (o->next)
     {
        _release_buffer(o, o->next);
        o->next = NULL;
        return EINA_TRUE;
     }
   if (!panic) return EINA_FALSE;

   /* This has been demoted to DBG from WRN because we
    * call this function to reclaim all buffers on a
    * surface resize.
    */
   DBG("Buffer release request when no next buffer");
   /* If we have to release these we're going to see tearing.
    * Try to reclaim in decreasing order of visual awfulness
    */
   if (o->current)
     {
        _release_buffer(o, o->current);
        o->current = NULL;
        return EINA_TRUE;
     }

   if (o->pending)
     {
        _release_buffer(o, o->pending);
        o->pending = NULL;
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

EAPI void *
ecore_drm2_fb_bo_get(Ecore_Drm2_Fb *fb)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fb, NULL);
   return fb->gbm_bo;
}
