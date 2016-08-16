#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include "eolian_database.h"
#include "eo_lexer.h"

EAPI const Eolian_Typedecl *
eolian_typedecl_alias_get_by_name(const char *name)
{
   if (!_aliases) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(name);
   Eolian_Typedecl *tp = eina_hash_find(_aliases, shr);
   eina_stringshare_del(shr);
   if (!tp) return NULL;
   return tp;
}

EAPI const Eolian_Typedecl *
eolian_typedecl_struct_get_by_name(const char *name)
{
   if (!_structs) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(name);
   Eolian_Typedecl *tp = eina_hash_find(_structs, shr);
   eina_stringshare_del(shr);
   if (!tp) return NULL;
   return tp;
}

EAPI const Eolian_Typedecl *
eolian_typedecl_enum_get_by_name(const char *name)
{
   if (!_enums) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(name);
   Eolian_Typedecl *tp = eina_hash_find(_enums, shr);
   eina_stringshare_del(shr);
   if (!tp) return NULL;
   return tp;
}

EAPI Eina_Iterator *
eolian_typedecl_aliases_get_by_file(const char *fname)
{
   if (!_aliasesf) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(fname);
   Eina_List *l = eina_hash_find(_aliasesf, shr);
   eina_stringshare_del(shr);
   if (!l) return NULL;
   return eina_list_iterator_new(l);
}

EAPI Eina_Iterator *
eolian_typedecl_structs_get_by_file(const char *fname)
{
   if (!_structsf) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(fname);
   Eina_List *l = eina_hash_find(_structsf, shr);
   eina_stringshare_del(shr);
   if (!l) return NULL;
   return eina_list_iterator_new(l);
}

EAPI Eina_Iterator *
eolian_typedecl_enums_get_by_file(const char *fname)
{
   if (!_enumsf) return NULL;
   Eina_Stringshare *shr = eina_stringshare_add(fname);
   Eina_List *l = eina_hash_find(_enumsf, shr);
   eina_stringshare_del(shr);
   if (!l) return NULL;
   return eina_list_iterator_new(l);
}

EAPI Eina_Iterator *
eolian_typedecl_all_aliases_get(void)
{
   return (_aliases ? eina_hash_iterator_data_new(_aliases) : NULL);
}

EAPI Eina_Iterator *
eolian_typedecl_all_structs_get(void)
{
   return (_structs ? eina_hash_iterator_data_new(_structs) : NULL);
}

EAPI Eina_Iterator *
eolian_typedecl_all_enums_get(void)
{
   return (_enums ? eina_hash_iterator_data_new(_enums) : NULL);
}

EAPI Eolian_Type_Type
eolian_type_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EOLIAN_TYPE_UNKNOWN_TYPE);
   return tp->type;
}

EAPI Eolian_Typedecl_Type
eolian_typedecl_type_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EOLIAN_TYPEDECL_UNKNOWN);
   return tp->type;
}

EAPI Eina_Iterator *
eolian_typedecl_struct_fields_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_STRUCT)
     return NULL;
   return eina_list_iterator_new(tp->field_list);
}

EAPI const Eolian_Struct_Type_Field *
eolian_typedecl_struct_field_get(const Eolian_Typedecl *tp, const char *field)
{
   Eolian_Struct_Type_Field *sf = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(field, NULL);
   if (tp->type != EOLIAN_TYPEDECL_STRUCT)
     return NULL;
   sf = eina_hash_find(tp->fields, field);
   if (!sf) return NULL;
   return sf;
}

EAPI Eina_Stringshare *
eolian_typedecl_struct_field_name_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->name;
}

EAPI const Eolian_Documentation *
eolian_typedecl_struct_field_documentation_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->doc;
}

EAPI const Eolian_Type *
eolian_typedecl_struct_field_type_get(const Eolian_Struct_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->type;
}

EAPI Eina_Iterator *
eolian_typedecl_enum_fields_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   return eina_list_iterator_new(tp->field_list);
}

EAPI const Eolian_Enum_Type_Field *
eolian_typedecl_enum_field_get(const Eolian_Typedecl *tp, const char *field)
{
   Eolian_Enum_Type_Field *ef = NULL;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(field, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   ef = eina_hash_find(tp->fields, field);
   if (!ef) return NULL;
   return ef;
}

EAPI Eina_Stringshare *
eolian_typedecl_enum_field_name_get(const Eolian_Enum_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->name;
}

EAPI Eina_Stringshare *
eolian_typedecl_enum_field_c_name_get(const Eolian_Enum_Type_Field *fl)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   char *bufp, *p;
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   buf = eina_strbuf_new();
   if (fl->base_enum->legacy)
     eina_strbuf_append(buf, fl->base_enum->legacy);
   else
     eina_strbuf_append(buf, fl->base_enum->full_name);
   eina_strbuf_append_char(buf, '_');
   eina_strbuf_append(buf, fl->name);
   bufp = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   eina_str_toupper(&bufp);
   while ((p = strchr(bufp, '.'))) *p = '_';
   ret = eina_stringshare_add(bufp);
   free(bufp);
   return ret;
}

EAPI const Eolian_Documentation *
eolian_typedecl_enum_field_documentation_get(const Eolian_Enum_Type_Field *fl)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   return fl->doc;
}

EAPI const Eolian_Expression *
eolian_typedecl_enum_field_value_get(const Eolian_Enum_Type_Field *fl, Eina_Bool force)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(fl, NULL);
   if (!force && !fl->is_public_value) return NULL;
   return fl->value;
}

EAPI Eina_Stringshare *
eolian_typedecl_enum_legacy_prefix_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (tp->type != EOLIAN_TYPEDECL_ENUM)
     return NULL;
   return tp->legacy;
}

EAPI const Eolian_Documentation *
eolian_typedecl_documentation_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->doc;
}

EAPI Eina_Stringshare *
eolian_type_file_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base.file;
}

EAPI Eina_Stringshare *
eolian_typedecl_file_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base.file;
}

EAPI const Eolian_Type *
eolian_type_base_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base_type;
}

EAPI const Eolian_Type *
eolian_type_next_type_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->next_type;
}

EAPI const Eolian_Typedecl *
eolian_type_typedecl_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_type_type_get(tp) != EOLIAN_TYPE_REGULAR)
     return NULL;
   /* try looking up if it belongs to a struct, enum or an alias... otherwise
    * return NULL, but first check for builtins
    */
   int  kw = eo_lexer_keyword_str_to_id(tp->full_name);
   if (!kw || kw < KW_byte || kw >= KW_true)
     {
        Eolian_Declaration *decl = eina_hash_find(_decls, tp->full_name);
        if (decl && decl->type != EOLIAN_DECL_CLASS
                 && decl->type != EOLIAN_DECL_VAR)
          return decl->data;
     }
   return NULL;
}

EAPI const Eolian_Type *
eolian_typedecl_base_type_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->base_type;
}

EAPI const Eolian_Type *
eolian_type_aliased_base_get(const Eolian_Type *tp)
{
   if (!tp || tp->type != EOLIAN_TYPE_REGULAR)
     return tp;
   const Eolian_Typedecl *btp = eolian_type_typedecl_get(tp);
   if (btp && (btp->type == EOLIAN_TYPEDECL_ALIAS))
     return eolian_typedecl_aliased_base_get(btp);
   return tp;
}

EAPI const Eolian_Type *
eolian_typedecl_aliased_base_get(const Eolian_Typedecl *tp)
{
   if (!tp || tp->type != EOLIAN_TYPEDECL_ALIAS)
     return NULL;
   return eolian_type_aliased_base_get(tp->base_type);
}

EAPI const Eolian_Class *
eolian_type_class_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (eolian_type_type_get(tp) != EOLIAN_TYPE_CLASS)
     return NULL;
   return eolian_class_get_by_name(tp->full_name);
}

EAPI size_t
eolian_type_array_size_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, 0);
   return tp->static_size;
}

EAPI Eina_Bool
eolian_type_is_own(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_own;
}

EAPI Eina_Bool
eolian_type_is_const(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_const;
}

EAPI Eina_Bool
eolian_type_is_ref(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_ref;
}

EAPI Eina_Bool
eolian_typedecl_is_extern(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, EINA_FALSE);
   return tp->is_extern;
}

EAPI Eina_Stringshare *
eolian_type_c_type_get(const Eolian_Type *tp)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   buf = eina_strbuf_new();
   database_type_to_str(tp, buf, NULL);
   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

EAPI Eina_Stringshare *
eolian_typedecl_c_type_get(const Eolian_Typedecl *tp)
{
   Eina_Stringshare *ret;
   Eina_Strbuf *buf;
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   buf = eina_strbuf_new();
   database_typedecl_to_str(tp, buf);
   ret = eina_stringshare_add(eina_strbuf_string_get(buf));
   eina_strbuf_free(buf);
   return ret;
}

EAPI Eina_Stringshare *
eolian_type_name_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->name;
}

EAPI Eina_Stringshare *
eolian_typedecl_name_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->name;
}

EAPI Eina_Stringshare *
eolian_type_full_name_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->full_name;
}

EAPI Eina_Stringshare *
eolian_typedecl_full_name_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->full_name;
}

EAPI Eina_Iterator *
eolian_type_namespaces_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (!tp->namespaces) return NULL;
   return eina_list_iterator_new(tp->namespaces);
}

EAPI Eina_Iterator *
eolian_typedecl_namespaces_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   if (!tp->namespaces) return NULL;
   return eina_list_iterator_new(tp->namespaces);
}

EAPI Eina_Stringshare *
eolian_type_free_func_get(const Eolian_Type *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->freefunc;
}

EAPI Eina_Stringshare *
eolian_typedecl_free_func_get(const Eolian_Typedecl *tp)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(tp, NULL);
   return tp->freefunc;
}
