#ifndef _EO_PRIVATE_H
#define _EO_PRIVATE_H

#include <Eo.h>
#include <Eina.h>

#define EO_EINA_MAGIC 0xa186bc32
#define EO_EINA_MAGIC_STR "Eo"
#define EO_FREED_EINA_MAGIC 0xa186bb32
#define EO_FREED_EINA_MAGIC_STR "Eo - Freed object"
#define EO_CLASS_EINA_MAGIC 0xa186ba32
#define EO_CLASS_EINA_MAGIC_STR "Efl Class"

#define EO_MAGIC_RETURN_VAL(d, magic, ret) \
   do { \
        if (!EINA_MAGIC_CHECK(d, magic)) \
          { \
             EINA_MAGIC_FAIL(d, magic); \
             return ret; \
          } \
   } while (0)

#define EO_MAGIC_RETURN(d, magic) \
   do { \
        if (!EINA_MAGIC_CHECK(d, magic)) \
          { \
             EINA_MAGIC_FAIL(d, magic); \
             return; \
          } \
   } while (0)


extern int _eo_log_dom;

#ifdef CRI
#undef CRI
#endif
#define CRI(...) EINA_LOG_DOM_CRIT(_eo_log_dom, __VA_ARGS__)

#ifdef ERR
#undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_eo_log_dom, __VA_ARGS__)

#ifdef WRN
#undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_eo_log_dom, __VA_ARGS__)

#ifdef INF
#undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_eo_log_dom, __VA_ARGS__)

#ifdef DBG
#undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_eo_log_dom, __VA_ARGS__)

typedef uintptr_t Eo_Id;
typedef struct _Efl_Class _Efl_Class;
typedef struct _Eo_Header Eo_Header;

/* Allocates an entry for the given object */
static inline Eo_Id _eo_id_allocate(const _Eo_Object *obj, const Eo *parent_id);

/* Releases an entry by the object id */
static inline void _eo_id_release(const Eo_Id obj_id);

void _eo_condtor_done(Eo *obj);

typedef struct _Dich_Chain1 Dich_Chain1;

typedef struct _Eo_Vtable
{
   Dich_Chain1 *chain;
   unsigned int size;
} Eo_Vtable;

/* Clean the vtable. */
void _vtable_func_clean_all(Eo_Vtable *vtable);

struct _Eo_Header
{
#ifndef HAVE_EO_ID
     EINA_MAGIC
#endif
     Eo_Id id;
};

struct _Eo_Object
{
     Eo_Header header;
     EINA_INLIST;
     const _Efl_Class *klass;
#ifdef EO_DEBUG
     Eina_Inlist *xrefs;
     Eina_Inlist *data_xrefs;
#endif

     Eo_Vtable *vtable;

     Eina_List *composite_objects;
     Efl_Del_Intercept del_intercept;

     short refcount;
     short user_refcount;
#ifdef EO_DEBUG
     short datarefcount;
#endif

     Eina_Bool condtor_done:1;
     Eina_Bool finalized:1;

     Eina_Bool del_triggered:1;
     Eina_Bool destructed:1;
     Eina_Bool manual_free:1;
};

/* How we search and store the implementations in classes. */
#define DICH_CHAIN_LAST_BITS 5
#define DICH_CHAIN_LAST_SIZE (1 << DICH_CHAIN_LAST_BITS)
#define DICH_CHAIN1(x) ((x) >> DICH_CHAIN_LAST_BITS)
#define DICH_CHAIN_LAST(x) ((x) & ((1 << DICH_CHAIN_LAST_BITS) - 1))

typedef void (*Eo_Op_Func_Type)(Eo *, void *class_data);

typedef struct
{
   Eo_Op_Func_Type func;
   const _Efl_Class *src;
} op_type_funcs;

typedef struct _Dich_Chain2
{
   op_type_funcs funcs[DICH_CHAIN_LAST_SIZE];
   unsigned short refcount;
} Dich_Chain2;

struct _Dich_Chain1
{
   Dich_Chain2 *chain2;
};

typedef struct
{
   const _Efl_Class *klass;
   size_t offset;
} Eo_Extension_Data_Offset;

struct _Efl_Class
{
   Eo_Header header;

   const _Efl_Class *parent;
   const Efl_Class_Description *desc;
   Eo_Vtable vtable;

   const _Efl_Class **extensions;

   Eo_Extension_Data_Offset *extn_data_off;

   const _Efl_Class **mro;

   /* cached object for faster allocation */
   struct {
      Eina_Trash  *trash;
      Eina_Spinlock    trash_lock;
      unsigned int trash_count;
   } objects;

   /* cached iterator for faster allocation cycle */
   struct {
      Eina_Trash   *trash;
      Eina_Spinlock trash_lock;
      unsigned int  trash_count;
   } iterators;

   unsigned int obj_size; /**< size of an object of this class */
   unsigned int base_id;
   unsigned int data_offset; /* < Offset of the data within object data. */
   unsigned int ops_count; /* < Offset of the data within object data. */

   Eina_Bool constructed : 1;
   Eina_Bool functions_set : 1;
   /* [extensions*] + NULL */
   /* [mro*] + NULL */
   /* [extensions data offset] + NULL */
};

typedef struct
{
   EINA_INLIST;
   const Eo *ref_obj;
   const char *data_klass;
   const char *file;
   int line;
} Eo_Xref_Node;

/* provide valgrind-like tracking of object allocationg and deletion */
void _eo_log_obj_report(const Eo_Id id, int log_level, const char *func_name, const char *file, int line);

void _efl_object_parent_sink_set(Eo *obj, Eina_Bool sink);

static inline
Eo *_eo_header_id_get(const Eo_Header *header)
{
#ifdef HAVE_EO_ID
   return (Eo *) header->id;
#else
   return (Eo *) header;
#endif
}

/* Retrieves the pointer to the object from the id */
_Eo_Object *_eo_obj_pointer_get(const Eo_Id obj_id, const char *func_name, const char *file, int line);

static inline
Efl_Class *_eo_class_id_get(const _Efl_Class *klass)
{
   return _eo_header_id_get((Eo_Header *) klass);
}

static inline
Eo *_eo_obj_id_get(const _Eo_Object *obj)
{
   return _eo_header_id_get((Eo_Header *) obj);
}

static inline void
_eo_condtor_reset(_Eo_Object *obj)
{
   obj->condtor_done = EINA_FALSE;
}

static inline void
_efl_del_internal(_Eo_Object *obj, const char *func_name, const char *file, int line)
{
   /* We need that for the event callbacks that may ref/unref. */
   obj->refcount++;

   const _Efl_Class *klass = obj->klass;

   efl_event_callback_call(_eo_obj_id_get(obj), EFL_EVENT_DEL, NULL);

   _eo_condtor_reset(obj);

   efl_destructor(_eo_obj_id_get(obj));

   if (!obj->condtor_done)
     {
        ERR("in %s:%d: func '%s' Object of class '%s' - Not all of the object destructors have been executed.",
            file, line, func_name, klass->desc->name);
     }
   /*FIXME: add eo_class_unref(klass) ? - just to clear the caches. */

     {
        Eina_List *itr, *itr_n;
        Eo *emb_obj;
        EINA_LIST_FOREACH_SAFE(obj->composite_objects, itr, itr_n, emb_obj)
          {
             efl_composite_detach(_eo_obj_id_get(obj), emb_obj);
          }
     }

   obj->destructed = EINA_TRUE;
   obj->refcount--;
}

static inline Eina_Bool
_obj_is_override(_Eo_Object *obj)
{
   return (obj->vtable != &obj->klass->vtable);
}

void _eo_free(_Eo_Object *obj, Eina_Bool manual_free);

static inline _Eo_Object *
_efl_ref(_Eo_Object *obj)
{
   obj->refcount++;
   return obj;
}

#define _efl_unref(obj) _efl_unref_internal(obj, __FUNCTION__, __FILE__, __LINE__)
static inline void
_efl_unref_internal(_Eo_Object *obj, const char *func_name, const char *file, int line)
{
   --(obj->refcount);
   if (EINA_UNLIKELY(obj->refcount <= 0))
     {
        if (obj->refcount < 0)
          {
             ERR("in %s:%d: func '%s' Obj:%p. Refcount (%d) < 0. Too many unrefs.", file, line, func_name, obj, obj->refcount);
             _eo_log_obj_report((Eo_Id)_eo_obj_id_get(obj), EINA_LOG_LEVEL_ERR, __FUNCTION__, __FILE__, __LINE__);
             return;
          }

        if (obj->destructed)
          {
             ERR("in %s:%d: func '%s' Object %p already destructed.", file, line, func_name, _eo_obj_id_get(obj));
             _eo_log_obj_report((Eo_Id)_eo_obj_id_get(obj), EINA_LOG_LEVEL_ERR, __FUNCTION__, __FILE__, __LINE__);
             return;
          }

        if (obj->del_triggered)
          {
             ERR("in %s:%d: func '%s' Object %p deletion already triggered. You wrongly call efl_unref() within a destructor.", file, line, func_name, _eo_obj_id_get(obj));
             _eo_log_obj_report((Eo_Id)_eo_obj_id_get(obj), EINA_LOG_LEVEL_ERR, __FUNCTION__, __FILE__, __LINE__);
             return;
          }

        if (obj->del_intercept)
          {
             Eo *obj_id = _eo_obj_id_get(obj);
             efl_ref(obj_id);
             obj->del_intercept(obj_id);
             return;
          }

        obj->del_triggered = EINA_TRUE;

        _efl_del_internal(obj, func_name, file, line);

        if (EINA_LIKELY(!obj->manual_free))
          {
#ifdef EO_DEBUG
             /* If for some reason it's not empty, clear it. */
             Eo *obj_id = _eo_obj_id_get(obj);
             while (obj->xrefs)
               {
                  Eina_Inlist *nitr = obj->xrefs->next;
                  Eo_Xref_Node *xref = EINA_INLIST_CONTAINER_GET(obj->data_xrefs, Eo_Xref_Node);
                  ERR("in %s:%d: func '%s' Object %p is still referenced by object %p. Origin: %s:%d",
                      file, line, func_name, obj_id, xref->ref_obj, xref->file, xref->line);
                  eina_freeq_ptr_main_add(xref, free, sizeof(*xref));
                  obj->xrefs = nitr;
               }
             while (obj->data_xrefs)
               {
                  Eina_Inlist *nitr = obj->data_xrefs->next;
                  Eo_Xref_Node *xref = EINA_INLIST_CONTAINER_GET(obj->data_xrefs, Eo_Xref_Node);
                  if (obj_id == xref->ref_obj)
                    {
                       WRN("in %s:%d: func '%s' Object %p still has a reference to its own data (subclass: %s). Origin: %s:%d",
                           file, line, func_name, obj_id, xref->data_klass, xref->file, xref->line);
                    }
                  else
                    {
                       ERR("in %s:%d: func '%s' Data of object %p (subclass: %s) is still referenced by object %p. Origin: %s:%d",
                           file, line, func_name, obj_id, xref->data_klass, xref->ref_obj, xref->file, xref->line);
                    }

                  eina_freeq_ptr_main_add(xref, free, sizeof(*xref));
                  obj->data_xrefs = nitr;
               }
#endif

             _eo_free(obj, EINA_FALSE);
          }
        else
          _efl_ref(obj); /* If we manual free, we keep a phantom ref. */
     }
}

Eina_Bool efl_future_init(void);
Eina_Bool efl_future_shutdown(void);

#endif
