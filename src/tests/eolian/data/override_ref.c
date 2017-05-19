EOAPI EFL_VOID_FUNC_BODY(override_a_set);

void _override_a_get(Eo *obj, Override_Data *pd);

EOAPI EFL_VOID_FUNC_BODY_CONST(override_a_get);

static void __eolian_override_b_set(Eo *obj EINA_UNUSED, Override_Data *pd, int idx EINA_UNUSED, float a, char b, int c)
{
   a = pd->a;
   b = pd->b;
   c = pd->c;
}

EOAPI EFL_VOID_FUNC_BODYV(override_b_set, EFL_FUNC_CALL(idx, a, b, c), int idx, float a, char b, int c);

static void __eolian_override_b_get(Eo *obj EINA_UNUSED, Override_Data *pd, int idx EINA_UNUSED, float *a, char *b, int *c)
{
   if (a) *a = pd->a;
   if (b) *b = pd->b;
   if (c) *c = pd->c;
}

EOAPI EFL_VOID_FUNC_BODYV_CONST(override_b_get, EFL_FUNC_CALL(idx, a, b, c), int idx, float *a, char *b, int *c);

static void __eolian_override_c_set(Eo *obj EINA_UNUSED, Override_Data *pd, int idx EINA_UNUSED, int c)
{
   c = pd->c;
}

EOAPI EFL_VOID_FUNC_BODYV(override_c_set, EFL_FUNC_CALL(idx, c), int idx, int c);

static int __eolian_override_c_get(Eo *obj EINA_UNUSED, Override_Data *pd EINA_UNUSED, int idx EINA_UNUSED)
{
   return 50;
}

EOAPI EFL_FUNC_BODYV_CONST(override_c_get, int, 50, EFL_FUNC_CALL(idx), int idx);
EOAPI EFL_VOID_FUNC_BODY(override_foo);

static void __eolian_override_bar(Eo *obj EINA_UNUSED, Override_Data *pd EINA_UNUSED, int idx EINA_UNUSED, int *a, char **str)
{
   if (a) *a = 250;
   if (str) *str = NULL;
}

EOAPI EFL_VOID_FUNC_BODYV(override_bar, EFL_FUNC_CALL(idx, a, str), int idx, int *a, char **str);

void _override_base_constructor(Eo *obj, Override_Data *pd);


static void __eolian_override_base_z_set(Eo *obj EINA_UNUSED, Override_Data *pd, int a, char b, float c)
{
   a = pd->a;
   b = pd->b;
   c = pd->c;
}


static void __eolian_override_base_z_get(Eo *obj EINA_UNUSED, Override_Data *pd, int *a, char *b, float *c)
{
   if (a) *a = pd->a;
   if (b) *b = pd->b;
   if (c) *c = pd->c;
}


static Eina_Bool
_override_class_initializer(Efl_Class *klass)
{
   EFL_OPS_DEFINE(ops,
      EFL_OBJECT_OP_FUNC(override_a_set, NULL),
      EFL_OBJECT_OP_FUNC(override_a_get, _override_a_get),
      EFL_OBJECT_OP_FUNC(override_b_set, __eolian_override_b_set),
      EFL_OBJECT_OP_FUNC(override_b_get, __eolian_override_b_get),
      EFL_OBJECT_OP_FUNC(override_c_set, __eolian_override_c_set),
      EFL_OBJECT_OP_FUNC(override_c_get, __eolian_override_c_get),
      EFL_OBJECT_OP_FUNC(override_foo, NULL),
      EFL_OBJECT_OP_FUNC(override_bar, __eolian_override_bar),
      EFL_OBJECT_OP_FUNC(base_constructor, _override_base_constructor),
      EFL_OBJECT_OP_FUNC(base_z_set, __eolian_override_base_z_set),
      EFL_OBJECT_OP_FUNC(base_z_get, __eolian_override_base_z_get)
   );
   return efl_class_functions_set(klass, &ops, NULL);
}

static const Efl_Class_Description _override_class_desc = {
   EO_VERSION,
   "Override",
   EFL_CLASS_TYPE_REGULAR,
   sizeof(Override_Data),
   _override_class_initializer,
   NULL,
   NULL
};

EFL_DEFINE_CLASS(override_class_get, &_override_class_desc, BASE_CLASS, NULL);
