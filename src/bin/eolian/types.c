#include "main.h"
#include "docs.h"

static Eina_Strbuf *
_type_generate(const Eolian_Unit *src, const Eolian_Typedecl *tp,
               Eina_Bool full, Eina_Bool legacy)
{
   char *grp = strdup(eolian_typedecl_full_name_get(tp));
   char *p = strrchr(grp, '.');
   if (p) *p = '\0';
   Eina_Strbuf *buf = eo_gen_docs_full_gen(src, eolian_typedecl_documentation_get(tp),
                                           grp, 0, legacy);
   free(grp);
   if (!buf) buf = eina_strbuf_new();
   else eina_strbuf_append_char(buf, '\n');
   Eolian_Typedecl_Type tpt = eolian_typedecl_type_get(tp);
   switch (tpt)
     {
      case EOLIAN_TYPEDECL_ALIAS:
        {
           Eina_Stringshare *tn = eolian_typedecl_c_type_get(src, tp);
           eina_strbuf_append(buf, tn);
           eina_stringshare_del(tn);
           break;
        }
      case EOLIAN_TYPEDECL_STRUCT:
      case EOLIAN_TYPEDECL_STRUCT_OPAQUE:
        {
           const Eolian_Struct_Type_Field *memb;
           char *fn = eo_gen_c_full_name_get(eolian_typedecl_full_name_get(tp));
           if (tpt == EOLIAN_TYPEDECL_STRUCT_OPAQUE || !full)
             {
                eina_strbuf_append_printf(buf, "typedef struct _%s %s", fn, fn);
                free(fn);
                break;
             }
           eina_strbuf_append_printf(buf, "typedef struct _%s\n{\n", fn);
           Eina_Iterator *membs = eolian_typedecl_struct_fields_get(tp);
           EINA_ITERATOR_FOREACH(membs, memb)
             {
                const Eolian_Type *mtp = eolian_typedecl_struct_field_type_get(memb);
                Eina_Stringshare *ct = NULL;
                if (eolian_type_type_get(mtp) == EOLIAN_TYPE_STATIC_ARRAY)
                  {
                     ct = eolian_type_c_type_get(eolian_type_base_type_get(mtp));
                     eina_strbuf_append_printf(buf, "  %s%s%s[%zu];",
                        ct, strchr(ct, '*') ? "" : " ",
                        eolian_typedecl_struct_field_name_get(memb),
                        eolian_type_array_size_get(mtp));
                  }
                else
                  {
                     ct = eolian_type_c_type_get(mtp);
                     eina_strbuf_append_printf(buf, "  %s%s%s;",
                        ct, strchr(ct, '*') ? "" : " ",
                        eolian_typedecl_struct_field_name_get(memb));
                  }
                eina_stringshare_del(ct);
                const Eolian_Documentation *fdoc
                   = eolian_typedecl_struct_field_documentation_get(memb);
                 if (fdoc)
                   {
                      const char *nl = strrchr(eina_strbuf_string_get(buf), '\n');
                      if (nl)
                        {
                           Eina_Strbuf *fbuf = eo_gen_docs_full_gen(src, fdoc, NULL,
                              strlen(nl), legacy);
                           if (fbuf)
                             eina_strbuf_append_printf(buf, " %s",
                                eina_strbuf_string_get(fbuf));
                           eina_strbuf_free(fbuf);
                        }
                   }
                eina_strbuf_append(buf, "\n");
             }
           eina_iterator_free(membs);
           eina_strbuf_append_printf(buf, "} %s", fn);
           free(fn);
           break;
        }
      case EOLIAN_TYPEDECL_ENUM:
        {
           const Eolian_Enum_Type_Field *memb;
           if (!full)
             break;
           eina_strbuf_append_printf(buf, "typedef enum\n{\n");
           Eina_Iterator *membs = eolian_typedecl_enum_fields_get(tp);
           Eina_Bool next = eina_iterator_next(membs, (void **)&memb);
           while (next)
             {
                const Eolian_Expression *vale =
                   eolian_typedecl_enum_field_value_get(memb, EINA_FALSE);
                Eina_Stringshare *membn =
                   eolian_typedecl_enum_field_c_name_get(memb);
                if (!vale)
                  eina_strbuf_append_printf(buf, "  %s", membn);
                else
                  {
                     Eolian_Value val =
                        eolian_expression_eval(src, vale, EOLIAN_MASK_INT);
                     const char *lit = eolian_expression_value_to_literal(&val);
                     eina_strbuf_append_printf(buf, "  %s = %s", membn, lit);
                     const char *exp = eolian_expression_serialize(vale);
                     if (exp && strcmp(lit, exp))
                       eina_strbuf_append_printf(buf, " /* %s */", exp);
                     eina_stringshare_del(exp);
                     eina_stringshare_del(lit);
                  }
                eina_stringshare_del(membn);
                const Eolian_Documentation *fdoc
                   = eolian_typedecl_enum_field_documentation_get(memb);
                next = eina_iterator_next(membs, (void **)&memb);
                if (next)
                  eina_strbuf_append(buf, ",");
                if (fdoc)
                  {
                     const char *nl = strrchr(eina_strbuf_string_get(buf), '\n');
                     if (nl)
                       {
                          Eina_Strbuf *fbuf = eo_gen_docs_full_gen(src, fdoc, NULL,
                             strlen(nl), legacy);
                          if (fbuf)
                            eina_strbuf_append_printf(buf, " %s",
                               eina_strbuf_string_get(fbuf));
                          eina_strbuf_free(fbuf);
                       }
                  }
                eina_strbuf_append(buf, "\n");
             }
           eina_iterator_free(membs);
           char *fn = eo_gen_c_full_name_get(eolian_typedecl_full_name_get(tp));
           eina_strbuf_append_printf(buf, "} %s", fn);
           free(fn);
           break;
        }
      case EOLIAN_TYPEDECL_FUNCTION_POINTER:
        {
           const Eolian_Function *fid = eolian_typedecl_function_pointer_get(tp);

           eina_strbuf_append(buf, "typedef ");

           /* Return type */
           const Eolian_Type *rtp = eolian_function_return_type_get(fid, EOLIAN_FUNCTION_POINTER);
           if (!rtp)
             eina_strbuf_append(buf, "void ");
           else
             {
                Eina_Stringshare *ct = eolian_type_c_type_get(rtp);
                eina_strbuf_append_printf(buf, "%s ", ct);
             }

           /* Function name */
           char *fn = eo_gen_c_full_name_get(eolian_typedecl_full_name_get(tp));
           eina_strbuf_append_printf(buf, "(*%s)", fn);
           free(fn);

           /* Parameters */
           eina_strbuf_append(buf, "(void *data");
           Eina_Iterator *params = eolian_function_parameters_get(fid);
           const Eolian_Function_Parameter *param = NULL;
           EINA_ITERATOR_FOREACH(params, param)
             {
                const Eolian_Typedecl *ptd = eolian_type_typedecl_get(eolian_parameter_type_get(param));
                Eina_Stringshare *pn = eolian_parameter_name_get(param);
                Eina_Stringshare *pt = eolian_type_c_type_get(eolian_parameter_type_get(param));

                if (!pn)
                  pn = ""; // FIXME add some kind of param1/param2 control for null?

                if (ptd && eolian_typedecl_type_get(ptd) == EOLIAN_TYPEDECL_FUNCTION_POINTER)
                  eina_strbuf_append_printf(buf, ", void *%s_data, %s %s, Eina_Free_Cb %s_free_cb",
                                            pn, pt, pn, pn);
                else
                  eina_strbuf_append_printf(buf, ", %s %s", pt, pn);

             }
           eina_strbuf_append(buf, ")");

           break;
        }
      default:
        eina_strbuf_reset(buf);
        break;
     }
   return buf;
}

static Eina_Strbuf *
_var_generate(const Eolian_Unit *src, const Eolian_Variable *vr, Eina_Bool legacy)
{
   char *fn = strdup(eolian_variable_full_name_get(vr));
   char *p = strrchr(fn, '.');
   if (p) *p = '\0';
   Eina_Strbuf *buf = eo_gen_docs_full_gen(src, eolian_variable_documentation_get(vr),
                                           fn, 0, legacy);
   if (p)
     {
        *p = '_';
        while ((p = strchr(fn, '.')))
          *p = '_';
     }
   eina_str_toupper(&fn);
   if (!buf) buf = eina_strbuf_new();
   else eina_strbuf_append_char(buf, '\n');
   const Eolian_Type *vt = eolian_variable_base_type_get(vr);
   if (eolian_variable_type_get(vr) == EOLIAN_VAR_CONSTANT)
     {
        /* we generate a define macro here, as it's a constant */
        eina_strbuf_prepend_printf(buf, "#ifndef %s\n", fn);
        eina_strbuf_append_printf(buf, "#define %s ", fn);
        const Eolian_Expression *vv = eolian_variable_value_get(vr);
        Eolian_Value val = eolian_expression_eval_type(src, vv, vt);
        Eina_Stringshare *lit = eolian_expression_value_to_literal(&val);
        eina_strbuf_append(buf, lit);
        Eina_Stringshare *exp = eolian_expression_serialize(vv);
        if (exp && strcmp(lit, exp))
          eina_strbuf_append_printf(buf, " /* %s */", exp);
        eina_stringshare_del(lit);
        eina_stringshare_del(exp);
        eina_strbuf_append(buf, "\n#endif");
     }
   else
     {
        Eina_Stringshare *ct = eolian_type_c_type_get(vt);
        eina_strbuf_append_printf(buf, "EWAPI extern %s %s;", ct, fn);
        eina_stringshare_del(ct);
     }
   free(fn);
   return buf;
}

void eo_gen_types_header_gen(const Eolian_Unit *src,
                             const char *eof, Eina_Strbuf *buf,
                             Eina_Bool full, Eina_Bool legacy)
{
   const Eolian_Declaration *decl;

   Eina_Iterator *itr = eolian_declarations_get_by_file(eof);
   EINA_ITERATOR_FOREACH(itr, decl)
     {
        Eolian_Declaration_Type dt = eolian_declaration_type_get(decl);

        if (dt == EOLIAN_DECL_VAR)
          {
             const Eolian_Variable *vr = eolian_declaration_variable_get(decl);
             if (!vr || eolian_variable_is_extern(vr))
               continue;

             Eina_Strbuf *vbuf = _var_generate(src, vr, legacy);
             if (vbuf)
               {
                  eina_strbuf_append(buf, eina_strbuf_string_get(vbuf));
                  eina_strbuf_append(buf, "\n\n");
                  eina_strbuf_free(vbuf);
               }
             continue;
          }

        if ((dt != EOLIAN_DECL_ALIAS) &&
            (dt != EOLIAN_DECL_STRUCT) &&
            (dt != EOLIAN_DECL_ENUM))
          continue;
        if (dt == EOLIAN_DECL_ENUM && !full)
          continue;

        const Eolian_Typedecl *tp = eolian_declaration_data_type_get(decl);
        if (!tp || eolian_typedecl_is_extern(tp))
          continue;

        if (eolian_typedecl_type_get(tp) == EOLIAN_TYPEDECL_ALIAS)
          {
             const Eolian_Type *btp = eolian_typedecl_base_type_get(tp);
             if (eolian_type_type_get(btp) == EOLIAN_TYPE_UNDEFINED)
               continue;
          }

        Eina_Strbuf *tbuf = _type_generate(src, tp, full, legacy);
        if (tbuf)
          {
             eina_strbuf_append(buf, eina_strbuf_string_get(tbuf));
             eina_strbuf_append(buf, ";\n\n");
             eina_strbuf_free(tbuf);
          }
     }
}

void eo_gen_types_source_gen(const Eolian_Unit *src,
                             const char *eof, Eina_Strbuf *buf)
{
   const Eolian_Declaration *decl;

   Eina_Iterator *itr = eolian_declarations_get_by_file(eof);
   EINA_ITERATOR_FOREACH(itr, decl)
     {
        Eolian_Declaration_Type dt = eolian_declaration_type_get(decl);

        if (dt != EOLIAN_DECL_VAR)
          continue;

        const Eolian_Variable *vr = eolian_declaration_variable_get(decl);
        if (!vr || eolian_variable_is_extern(vr))
          continue;

        if (eolian_variable_type_get(vr) == EOLIAN_VAR_CONSTANT)
          continue;

        const Eolian_Expression *vv = eolian_variable_value_get(vr);
        if (!vv)
          continue;

        char *fn = strdup(eolian_variable_full_name_get(vr));
        for (char *p = strchr(fn, '.'); p; p = strchr(p, '.'))
          *p = '_';
        eina_str_toupper(&fn);

        const Eolian_Type *vt = eolian_variable_base_type_get(vr);
        Eina_Stringshare *ct = eolian_type_c_type_get(vt);
        eina_strbuf_append_printf(buf, "EWAPI %s %s = ", ct, fn);
        eina_stringshare_del(ct);
        free(fn);

        Eolian_Value val = eolian_expression_eval_type(src, vv, vt);
        Eina_Stringshare *lit = eolian_expression_value_to_literal(&val);
        eina_strbuf_append(buf, lit);
        eina_strbuf_append_char(buf, ';');
        Eina_Stringshare *exp = eolian_expression_serialize(vv);
        if (exp && strcmp(lit, exp))
          eina_strbuf_append_printf(buf, " /* %s */", exp);
        eina_stringshare_del(lit);
        eina_stringshare_del(exp);

        eina_strbuf_append(buf, "\n");
     }
}

Eina_Strbuf *eo_gen_class_typedef_gen(const Eolian_Unit *src, const char *eof)
{
   const Eolian_Class *cl = eolian_class_get_by_file(src, eof);
   if (!cl)
     return NULL;
   char *clfn = eo_gen_c_full_name_get(eolian_class_full_name_get(cl));
   if (!clfn)
     return NULL;
   Eina_Strbuf *ret = eina_strbuf_new();
   eina_strbuf_append_printf(ret, "typedef Eo %s;\n", clfn);
   free(clfn);
   return ret;
}
