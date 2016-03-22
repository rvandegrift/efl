#define EVAS_FILTER_PROTECTED

#include "evas_common_private.h"
#include "evas_private.h"
#include "../../lib/efl/interfaces/efl_gfx_filter.eo.h"
#include "evas_filter.eo.h"
#include "evas_filter.h"

#define MY_CLASS EVAS_FILTER_MIXIN

#define ENFN obj->layer->evas->engine.func
#define ENDT obj->layer->evas->engine.data.output

typedef struct _Evas_Filter_Data Evas_Filter_Data;
struct _Evas_Filter_Data
{
   const Evas_Object_Filter_Data *data;
};

static void
_filter_cb(Evas_Filter_Context *ctx, void *data, Eina_Bool success)
{
   Eo *eo_obj = data;

   // Destroy context as we won't reuse it.
   evas_filter_context_destroy(ctx);

   // Redraw text with normal styles in case of failure
   if (!success)
     {
        Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);

        ERR("Filter failed at runtime!");
        eo_do(eo_obj,
              evas_filter_invalid_set(EINA_TRUE);
              evas_filter_dirty());
        evas_object_change(eo_obj, obj);
        evas_object_clip_dirty(eo_obj, obj);
        evas_object_coords_recalc(eo_obj, obj);
        evas_object_inform_call_resize(eo_obj);
     }
}

static void
_filter_source_hash_free_cb(void *data)
{
   Evas_Filter_Proxy_Binding *pb = data;
   Evas_Object_Protected_Data *proxy, *source;
   Evas_Filter_Data *pd;

   proxy = eo_data_scope_get(pb->eo_proxy, EVAS_OBJECT_CLASS);
   source = eo_data_scope_get(pb->eo_source, EVAS_OBJECT_CLASS);

   if (source)
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, source->proxy,
                             Evas_Object_Proxy_Data, source_write)
          source_write->proxies = eina_list_remove(source_write->proxies, pb->eo_proxy);
        EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, source_write)
     }

   pd = eo_data_scope_get(pb->eo_proxy, MY_CLASS);

   if (pd && proxy)
     {
        if (!eina_hash_population(pd->data->sources))
          {
             EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, proxy->proxy,
                                  Evas_Object_Proxy_Data, proxy_write)
               proxy_write->is_proxy = EINA_FALSE;
             EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, proxy_write)
          }
     }

   eina_stringshare_del(pb->name);
   free(pb);
}

#define FCOW_BEGIN(_pd) eina_cow_write(evas_object_filter_cow, (const Eina_Cow_Data**)&(_pd->data))
#define FCOW_END(_fcow, _pd) eina_cow_done(evas_object_filter_cow, (const Eina_Cow_Data**)&(_pd->data), _fcow, EINA_TRUE)

Eina_Bool
evas_filter_object_render(Eo *eo_obj, Evas_Object_Protected_Data *obj,
                          void *output, void *context, void *surface,
                          int x, int y, Eina_Bool do_async, Eina_Bool alpha)
{
   Evas_Filter_Data *pd = eo_data_scope_get(eo_obj, MY_CLASS);

   if (!pd->data->invalid && (pd->data->chain || pd->data->code))
     {
        int X, Y, W, H, l = 0, r = 0, t = 0, b = 0;
        Evas_Filter_Context *filter;
        void *drawctx;
        Eina_Bool ok;
        void *previous = pd->data->output;
        Evas_Object_Filter_Data *fcow;
        void *filter_output;

        /* NOTE: Filter rendering is now done ENTIRELY on CPU.
         * So we rely on cache/cache2 to allocate a real image buffer,
         * that we can draw to. The OpenGL texture will be created only
         * after the rendering has been done, as we simply push the output
         * image to GL.
         */

        W = obj->cur->geometry.w;
        H = obj->cur->geometry.h;
        X = obj->cur->geometry.x;
        Y = obj->cur->geometry.y;

        // Prepare color multiplier
        ENFN->context_color_set(output, context,
                                obj->cur->cache.clip.r,
                                obj->cur->cache.clip.g,
                                obj->cur->cache.clip.b,
                                obj->cur->cache.clip.a);
        if (obj->cur->clipper)
          ENFN->context_multiplier_set(output, context,
                                       obj->cur->clipper->cur->cache.clip.r,
                                       obj->cur->clipper->cur->cache.clip.g,
                                       obj->cur->clipper->cur->cache.clip.b,
                                       obj->cur->clipper->cur->cache.clip.a);
        else
          ENFN->context_multiplier_unset(output, context);

        if (!pd->data->chain)
          {
             Evas_Filter_Program *pgm;
             pgm = evas_filter_program_new(pd->data->name, alpha);
             evas_filter_program_source_set_all(pgm, pd->data->sources);
             evas_filter_program_data_set_all(pgm, pd->data->data);
             evas_filter_program_state_set(pgm, eo_obj, obj,
                                           pd->data->state.cur.name, pd->data->state.cur.value,
                                           pd->data->state.next.name, pd->data->state.next.value,
                                           pd->data->state.pos);
             if (!evas_filter_program_parse(pgm, pd->data->code))
               {
                  ERR("Filter program parsing failed");
                  evas_filter_program_del(pgm);

                  if (!pd->data->invalid)
                    {
                       fcow = FCOW_BEGIN(pd);
                       fcow->invalid = EINA_TRUE;
                       FCOW_END(fcow, pd);
                    }

                  return EINA_FALSE;
               }
             fcow = FCOW_BEGIN(pd);
             fcow->chain = pgm;
             fcow->invalid = EINA_FALSE;
             FCOW_END(fcow, pd);
          }
        else if (previous && !pd->data->changed)
          {
             Eina_Bool redraw;

             redraw = evas_filter_program_state_set(pd->data->chain, eo_obj, obj,
                                                    pd->data->state.cur.name, pd->data->state.cur.value,
                                                    pd->data->state.next.name, pd->data->state.next.value,
                                                    pd->data->state.pos);
             if (redraw)
               DBG("Filter redraw by state change!");
             else if (obj->changed)
               {
                  // FIXME: Check that something else than X,Y changed!
                  redraw = EINA_TRUE;
                  DBG("Filter redraw by object content change!");
               }

             // Scan proxies to find if any changed
             if (!redraw && pd->data->sources)
               {
                  Evas_Filter_Proxy_Binding *pb;
                  Evas_Object_Protected_Data *source;
                  Eina_Iterator *iter;

                  iter = eina_hash_iterator_data_new(pd->data->sources);
                  EINA_ITERATOR_FOREACH(iter, pb)
                    {
                       source = eo_data_scope_get(pb->eo_source, EVAS_OBJECT_CLASS);
                       if (source->changed)
                         {
                            redraw = EINA_TRUE;
                            break;
                         }
                    }
                  eina_iterator_free(iter);
               }

             if (!redraw)
               {
                  // Render this image only
                  ENFN->image_draw(ENDT, context,
                                   surface, previous,
                                   0, 0, W, H,         // src
                                   X + x, Y + y, W, H, // dst
                                   EINA_FALSE,         // smooth
                                   do_async);
                  return EINA_TRUE;
               }
          }
        else
           evas_filter_program_state_set(pd->data->chain, eo_obj, obj,
                                         pd->data->state.cur.name, pd->data->state.cur.value,
                                         pd->data->state.next.name, pd->data->state.next.value,
                                         pd->data->state.pos);

        filter = evas_filter_context_new(obj->layer->evas, do_async);

        // Run script
        ok = evas_filter_context_program_use(filter, pd->data->chain);
        if (!filter || !ok)
          {
             ERR("Parsing failed?");
             evas_filter_context_destroy(filter);

             if (!pd->data->invalid)
               {
                  fcow = FCOW_BEGIN(pd);
                  fcow->invalid = EINA_TRUE;
                  FCOW_END(fcow, pd);
               }
             return EINA_FALSE;
          }

        // Proxies
        evas_filter_context_proxy_render_all(filter, eo_obj, EINA_FALSE);

        // Draw Context
        drawctx = ENFN->context_new(ENDT);
        ENFN->context_color_set(ENDT, drawctx, 255, 255, 255, 255);

        // Allocate all buffers now
        evas_filter_context_buffers_allocate_all(filter);
        evas_filter_target_set(filter, context, surface, X + x, Y + y);

        // Steal output and release previous
        filter_output = evas_filter_buffer_backing_steal(filter, EVAS_FILTER_BUFFER_OUTPUT_ID);
        evas_filter_buffer_backing_release(filter, previous);

        // Request rendering from the object itself (child class)
        evas_filter_program_padding_get(pd->data->chain, &l, &r, &t, &b);
        eo_do(eo_obj, evas_filter_input_render(filter, drawctx, l, r, t, b, do_async));

        ENFN->context_free(ENDT, drawctx);

        // Add post-run callback and run filter
        evas_filter_context_post_run_callback_set(filter, _filter_cb, eo_obj);
        ok = evas_filter_run(filter);

        fcow = FCOW_BEGIN(pd);
        fcow->output = filter_output;
        fcow->changed = EINA_FALSE;
        fcow->async = do_async;
        if (!ok) fcow->invalid = EINA_TRUE;
        FCOW_END(fcow, pd);

        if (ok)
          {
             DBG("Effect rendering done.");
             return EINA_TRUE;
          }
        else
          {
             ERR("Rendering failed.");
             return EINA_FALSE;
          }
     }
   return EINA_FALSE;
}

EOLIAN void
_evas_filter_efl_gfx_filter_program_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                        const char *code, const char *name)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Filter_Program *pgm = NULL;
   Evas_Object_Filter_Data *fcow;
   Eina_Bool alpha;

   if (!pd) return;
   if ((pd->data->code == code) && (!name || (pd->data->name == name))) return;
   if (pd->data->code && code && !strcmp(code, pd->data->code) &&
       pd->data->name && name && !strcmp(name, pd->data->name)) return;

   evas_object_async_block(obj);
   fcow = FCOW_BEGIN(pd);
     {
        // Parse filter program
        evas_filter_program_del(fcow->chain);
        eina_stringshare_replace(&fcow->name, name);
        if (code)
          {
             eo_do(eo_obj, alpha = evas_filter_input_alpha());
             pgm = evas_filter_program_new(fcow->name, alpha);
             evas_filter_program_source_set_all(pgm, fcow->sources);
             evas_filter_program_data_set_all(pgm, fcow->data);
             evas_filter_program_state_set(pgm, eo_obj, obj,
                                           fcow->state.cur.name, fcow->state.cur.value,
                                           fcow->state.next.name, fcow->state.next.value,
                                           fcow->state.pos);
             if (!evas_filter_program_parse(pgm, code))
               {
                  ERR("Parsing failed!");
                  evas_filter_program_del(pgm);
                  pgm = NULL;
               }
          }
        fcow->chain = pgm;
        fcow->changed = EINA_TRUE;
        fcow->invalid = (pgm == NULL);
        eina_stringshare_replace(&fcow->code, code);
     }
   FCOW_END(fcow, pd);

   // Update object
   eo_do(eo_obj, evas_filter_dirty());
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_coords_recalc(eo_obj, obj);
   evas_object_inform_call_resize(eo_obj);
}

EOLIAN void
_evas_filter_efl_gfx_filter_program_get(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, const char **code, const char **name)
{
   if (code) *code = pd->data->code;
   if (name) *name = pd->data->name;
}

EOLIAN void
_evas_filter_efl_gfx_filter_source_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                       const char *name, Efl_Gfx_Base *eo_source)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Filter_Proxy_Binding *pb, *pb_old = NULL;
   Evas_Object_Protected_Data *source = NULL;
   Evas_Object_Filter_Data *fcow = NULL;

   if (eo_source)
     source = eo_data_scope_get(eo_source, EVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if (!name)
     {
        if (!eo_source || !pd->data->sources) return;
        if (eina_hash_del_by_data(pd->data->sources, eo_source))
          goto update;
        return;
     }

   if (!source && !pd->data->sources)
     return;

   if (pd->data->sources)
     {
        pb_old = eina_hash_find(pd->data->sources, name);
        if (pb_old && (pb_old->eo_source == eo_source)) return;
     }

   fcow = FCOW_BEGIN(pd);
   if (!fcow->sources)
     fcow->sources = eina_hash_string_small_new(EINA_FREE_CB(_filter_source_hash_free_cb));
   else if (pb_old)
     eina_hash_del(fcow->sources, name, pb_old);

   if (!source)
     {
        pb_old = eina_hash_find(fcow->sources, name);
        if (!pb_old)
          {
             FCOW_END(fcow, pd);
             return;
          }
        eina_hash_del_by_key(fcow->sources, name);
        goto update;
     }

   pb = calloc(1, sizeof(*pb));
   pb->eo_proxy = eo_obj;
   pb->eo_source = eo_source;
   pb->name = eina_stringshare_add(name);

   if (!eina_list_data_find(source->proxy->proxies, eo_obj))
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, source->proxy, Evas_Object_Proxy_Data, source_write)
          source_write->proxies = eina_list_append(source_write->proxies, eo_obj);
        EINA_COW_WRITE_END(evas_object_proxy_cow, source->proxy, source_write)
     }

   if (!obj->proxy->is_proxy)
     {
        EINA_COW_WRITE_BEGIN(evas_object_proxy_cow, obj->proxy, Evas_Object_Proxy_Data, proxy_write)
          proxy_write->is_proxy = EINA_TRUE;
        EINA_COW_WRITE_END(evas_object_proxy_cow, obj->proxy, proxy_write)
     }

   eina_hash_add(fcow->sources, pb->name, pb);
   evas_filter_program_source_set_all(fcow->chain, fcow->sources);
   evas_filter_program_data_set_all(fcow->chain, fcow->data);

   // Update object
update:
   if (fcow)
     {
        fcow->changed = EINA_TRUE;
        fcow->invalid = EINA_FALSE;
        FCOW_END(fcow, pd);
     }

   eo_do(eo_obj, evas_filter_dirty());
   evas_object_change(eo_obj, obj);
   evas_object_clip_dirty(eo_obj, obj);
   evas_object_coords_recalc(eo_obj, obj);
   evas_object_inform_call_resize(eo_obj);
}

EOLIAN void
_evas_filter_efl_gfx_filter_source_get(const Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd,
                                       const char *name, Efl_Gfx_Base **source)
{
   if (!source) return;
   *source = eina_hash_find(pd->data->sources, name);
}

EOLIAN void
_evas_filter_efl_gfx_filter_state_set(Eo *eo_obj, Evas_Filter_Data *pd,
                                      const char *cur_state, double cur_val,
                                      const char *next_state, double next_val,
                                      double pos)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);

   evas_object_async_block(obj);
   if ((cur_state != pd->data->state.cur.name) || (cur_val != pd->data->state.cur.value) ||
       (next_state != pd->data->state.next.name) || (next_val != pd->data->state.next.value) ||
       (pos != pd->data->state.pos))
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->changed = 1;
        fcow->state.cur.name = cur_state;
        fcow->state.cur.value = cur_val;
        fcow->state.next.name = next_state;
        fcow->state.next.value = next_val;
        fcow->state.pos = pos;
        FCOW_END(fcow, pd);

        if (pd->data->chain)
          {
             evas_filter_program_state_set(pd->data->chain, eo_obj, obj,
                                           pd->data->state.cur.name, pd->data->state.cur.value,
                                           pd->data->state.next.name, pd->data->state.next.value,
                                           pd->data->state.pos);
          }

        // Mark as changed
        eo_do(eo_obj, evas_filter_dirty());
        evas_object_change(eo_obj, obj);
        evas_object_clip_dirty(eo_obj, obj);
        evas_object_coords_recalc(eo_obj, obj);
        evas_object_inform_call_resize(eo_obj);
     }
}

EOLIAN void
_evas_filter_efl_gfx_filter_padding_get(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd,
                                        int *l, int *r, int *t, int *b)
{
   if (!pd->data->chain)
     {
        if (l) *l = 0;
        if (r) *r = 0;
        if (t) *t = 0;
        if (b) *b = 0;
        return;
     }
   evas_filter_program_padding_get(pd->data->chain, l, r, t, b);
}

EOLIAN void
_evas_filter_changed_set(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, Eina_Bool val)
{
   if ((evas_object_filter_cow_default != pd->data) && (pd->data->changed != val))
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->changed = val;
        FCOW_END(fcow, pd);
     }
}

EOLIAN void
_evas_filter_invalid_set(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd, Eina_Bool val)
{
   if (pd->data->invalid != val)
     {
        Evas_Object_Filter_Data *fcow = FCOW_BEGIN(pd);
        fcow->invalid = val;
        FCOW_END(fcow, pd);
     }
}

EOLIAN void
_evas_filter_constructor(Eo *eo_obj EINA_UNUSED, Evas_Filter_Data *pd)
{
   pd->data = eina_cow_alloc(evas_object_filter_cow);
}

EOLIAN void
_evas_filter_destructor(Eo *eo_obj, Evas_Filter_Data *pd)
{
   Evas_Object_Protected_Data *obj = eo_data_scope_get(eo_obj, EVAS_OBJECT_CLASS);
   Evas_Filter_Data_Binding *db;
   Eina_Inlist *il;

   if (!pd->data) return;
   if (evas_object_filter_cow_default == pd->data) return;

   if (pd->data->output)
     {
        if (!pd->data->async)
          ENFN->image_free(ENDT, pd->data->output);
        else
          evas_unref_queue_image_put(obj->layer->evas, pd->data->output);
     }
   eina_hash_free(pd->data->sources);
   EINA_INLIST_FOREACH_SAFE(pd->data->data, il, db)
     {
        eina_stringshare_del(db->name);
        eina_stringshare_del(db->value);
        free(db);
     }
   evas_filter_program_del(pd->data->chain);
   eina_stringshare_del(pd->data->code);
   eina_cow_free(evas_object_filter_cow, (const Eina_Cow_Data **) &pd->data);
}

EOLIAN void
_evas_filter_efl_gfx_filter_data_set(Eo *obj EINA_UNUSED, Evas_Filter_Data *pd,
                                     const char *name, const char *value,
                                     Eina_Bool execute)
{
   Evas_Filter_Data_Binding *db, *found = NULL;
   Evas_Object_Filter_Data *fcow;

   EINA_SAFETY_ON_NULL_RETURN(pd->data);
   EINA_SAFETY_ON_NULL_RETURN(name);

   EINA_INLIST_FOREACH(pd->data->data, db)
     {
        if (!strcmp(name, db->name))
          {
             if (db->execute == execute)
               {
                  if ((value == db->value) || (value && db->value && !strcmp(value, db->value)))
                    return;
               }
             found = db;
             break;
          }
     }

   fcow = FCOW_BEGIN(pd);
     {
        if (found)
          {
             // Note: we are keeping references to NULL values here.
             eina_stringshare_replace(&found->value, value);
             found->execute = execute;
          }
        else if (value)
          {
             db = calloc(1, sizeof(Evas_Filter_Data_Binding));
             db->name = eina_stringshare_add(name);
             db->value = eina_stringshare_add(value);
             db->execute = execute;
             fcow->data = eina_inlist_append(fcow->data, EINA_INLIST_GET(db));
          }
        evas_filter_program_data_set_all(fcow->chain, fcow->data);
        fcow->changed = 1;
     }
   FCOW_END(fcow, pd);
}

EOLIAN void *
_evas_filter_output_buffer_get(Eo *obj EINA_UNUSED, Evas_Filter_Data *pd)
{
   return pd->data->output;
}

#include "evas_filter.eo.c"
