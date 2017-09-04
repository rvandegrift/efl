#include "evas_common_private.h"
#include "evas_private.h"

static Evas_Version _version = { VMAJ, VMIN, VMIC, VREV };
EAPI Evas_Version *evas_version = &_version;

int _evas_alloc_error = 0;
static int _evas_debug_init = 0;
static enum {
     _EVAS_DEBUG_DEFAULT,
     _EVAS_DEBUG_HIDE,
     _EVAS_DEBUG_SHOW
} _evas_debug_show = _EVAS_DEBUG_DEFAULT;
static int _evas_debug_abort = 0;

EAPI Evas_Alloc_Error
evas_alloc_error(void)
{
   return _evas_alloc_error;
}

static void
_evas_debug_init_from_env(void)
{
   const char *tmp = getenv("EVAS_DEBUG_SHOW");
   if (tmp)
     {
        int dbgshow = atoi(tmp);
        _evas_debug_show = (dbgshow) ? _EVAS_DEBUG_SHOW : _EVAS_DEBUG_HIDE;
     }
   if (getenv("EVAS_DEBUG_ABORT")) _evas_debug_abort = 1;
   _evas_debug_init = 1;
}

void
evas_debug_error(void)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if (_evas_debug_show == _EVAS_DEBUG_SHOW)
     CRI("Evas Magic Check Failed!!!");
}

void
evas_debug_input_null(void)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if (_evas_debug_show == _EVAS_DEBUG_SHOW)
     CRI("Input object pointer is NULL!");
   if (_evas_debug_abort) abort();
}

void
evas_debug_magic_null(void)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if ((_evas_debug_show == _EVAS_DEBUG_SHOW) ||
         (_evas_debug_show == _EVAS_DEBUG_DEFAULT))
     CRI("Input object is zero'ed out (maybe a freed object or zero-filled RAM)!");
   if (_evas_debug_abort) abort();
}

void
evas_debug_magic_wrong(DATA32 expected, DATA32 supplied)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if ((_evas_debug_show == _EVAS_DEBUG_SHOW) ||
         (_evas_debug_show == _EVAS_DEBUG_DEFAULT))
     CRI("Input object is wrong type\n"
	  "    Expected: %08x - %s\n"
	  "    Supplied: %08x - %s",
	  expected, evas_debug_magic_string_get(expected),
	  supplied, evas_debug_magic_string_get(supplied));
   if (_evas_debug_abort) abort();
}

void
evas_debug_generic(const char *str)
{
   if (!_evas_debug_init)
     {
        _evas_debug_init_from_env();
     }
   if ((_evas_debug_show == _EVAS_DEBUG_SHOW) ||
         (_evas_debug_show == _EVAS_DEBUG_DEFAULT))
     CRI("%s", str);
   if (_evas_debug_abort) abort();
}

const char *
evas_debug_magic_string_get(DATA32 magic)
{
   switch (magic)
     {
      case MAGIC_EVAS:
	return "Evas";
      case MAGIC_OBJ:
	return "Evas_Object";
      case MAGIC_OBJ_RECTANGLE:
	return "Evas_Object (Rectangle)";
      case MAGIC_OBJ_LINE:
	return "Evas_Object (Line)";
      case MAGIC_OBJ_POLYGON:
	return "Evas_Object (Polygon)";
      case MAGIC_OBJ_IMAGE:
	return "Evas_Object (Image)";
      case MAGIC_OBJ_TEXT:
	return "Evas_Object (Text)";
      case MAGIC_OBJ_SMART:
	return "Evas_Object (Smart)";
      case MAGIC_EVAS_GL:
	return "Evas_GL";
      case MAGIC_MAP:
	return "Evas_Map";
      default:
	return "<UNKNOWN>";
     };
}
