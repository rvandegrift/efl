#include "evas_common_private.h"
#include "evas_private.h"

#include "evas_vg_private.h"
#include "efl_vg_root_node.eo.h"

#include <string.h>

#define MY_CLASS EFL_VG_ROOT_NODE_CLASS

typedef struct _Efl_VG_Root_Node_Data Efl_VG_Root_Node_Data;
struct _Efl_VG_Root_Node_Data
{
   Evas_Object *parent;
   Evas_Object_Protected_Data *data;
};

static void
_evas_vg_root_node_render_pre(Eo *obj EINA_UNUSED,
                              Eina_Matrix3 *parent,
                              Ector_Surface *s,
                              void *data,
                              Efl_VG_Data *nd)
{
   Efl_VG_Container_Data *pd = data;
   Eina_List *l;
   Eo *child;

   EFL_VG_COMPUTE_MATRIX(current, parent, nd);

   EINA_LIST_FOREACH(pd->children, l, child)
     _evas_vg_render_pre(child, s, current);
}

static void
_evas_vg_root_node_changed(void *data, const Efl_Event *event)
{
   Efl_VG_Root_Node_Data *pd = data;
   Efl_VG_Data *bd = efl_data_scope_get(event->object, EFL_VG_CLASS);

   if (bd->changed) return;
   bd->changed = EINA_TRUE;

   if (pd->parent) evas_object_change(pd->parent, pd->data);
}

static void
_efl_vg_root_node_efl_object_parent_set(Eo *obj,
                                     Efl_VG_Root_Node_Data *pd,
                                     Eo *parent)
{
   // Nice little hack, jump over parent parent_set in Efl_VG_Root
   efl_parent_set(efl_super(obj, EFL_VG_CLASS), parent);
   if (parent && !efl_isa(parent, EVAS_VG_CLASS))
     {
        ERR("Parent of VG_ROOT_NODE must be a VG_CLASS");
     }
   else
     {
        pd->parent = parent;
        pd->data = parent ? efl_data_scope_get(parent, EFL_CANVAS_OBJECT_CLASS) : NULL;
     }
}

static Eo *
_efl_vg_root_node_efl_object_constructor(Eo *obj,
                                      Efl_VG_Root_Node_Data *pd)
{
   Efl_VG_Container_Data *cd;
   Efl_VG_Data *nd;
   Eo *parent;

   // We are copying here the code of the vg container to make it possible to
   // enforce that the root node is the only one to attach to an Evas_Object_VG
   cd = efl_data_scope_get(obj, EFL_VG_CONTAINER_CLASS);
   cd->children = NULL;
   cd->names = eina_hash_stringshared_new(NULL);

   // Nice little hack, jump over parent constructor in Efl_VG_Root
   obj = efl_constructor(efl_super(obj, EFL_VG_CLASS));
   parent = efl_parent_get(obj);
   efl_vg_name_set(obj, "root");
   if (!efl_isa(parent, EVAS_VG_CLASS)) {
        ERR("Parent of VG_ROOT_NODE must be a VG_CLASS");
        return NULL;
   }

   nd = efl_data_scope_get(obj, EFL_VG_CLASS);
   nd->render_pre = _evas_vg_root_node_render_pre;
   nd->data = cd;

   efl_event_callback_add(obj, EFL_GFX_EVENT_CHANGED, _evas_vg_root_node_changed, pd);

   return obj;
}

#include "efl_vg_root_node.eo.c"
