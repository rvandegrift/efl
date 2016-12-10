#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdlib.h>

#include <Eo.h>
#include <Ecore.h>

#include "simple.eo.h"

#define MY_CLASS SIMPLE_CLASS

static Eo *_simple_eo_base_constructor(Eo *obj, void *pd EINA_UNUSED)
{
   return eo_constructor(eo_super(obj, MY_CLASS));
}

static Eina_Bool _simple_simple_get(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED)
{
   printf("calling %s\n", __FUNCTION__);

   return EINA_TRUE;
}

static Eina_Bool _simple_name_get(Eo *obj EINA_UNUSED, void *pd EINA_UNUSED, const char** name)
{
   static const char* _name= "simple_class";

   printf("calling %s= simples class\n", __FUNCTION__);
   *name = _name;

   return EINA_TRUE;
}

#include "simple.eo.c"
