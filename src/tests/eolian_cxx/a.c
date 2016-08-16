#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Eo.h>

#include "a.eo.h"

struct _A_Data
{
  int callbacks;
};
typedef struct _A_Data A_Data;

#define MY_CLASS A_CLASS

static Eo *_a_eo_base_constructor(Eo *obj EINA_UNUSED, A_Data *pd EINA_UNUSED)
{
   return eo_constructor(eo_super(obj, MY_CLASS));
}

#include "a.eo.c"
