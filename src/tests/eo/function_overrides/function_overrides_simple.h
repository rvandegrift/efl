#ifndef SIMPLE_H
#define SIMPLE_H

typedef struct
{
   int a;
} Simple_Public_Data;

EAPI void simple_a_set(Eo *obj, int a);
EAPI Eina_Bool simple_a_print(Eo *obj);
EAPI Eina_Bool simple_class_print(const Eo *obj);
EAPI Eina_Bool simple_class_print2(const Eo *obj);

extern const Eo_Event_Description _SIG_A_CHANGED;
#define SIG_A_CHANGED (&(_SIG_A_CHANGED))

#define SIMPLE_CLASS simple_class_get()
const Eo_Class *simple_class_get(void);

#endif
