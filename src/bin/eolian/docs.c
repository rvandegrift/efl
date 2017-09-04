#include <ctype.h>

#include "docs.h"

static int
_indent_line(Eina_Strbuf *buf, int ind)
{
   int i;
   for (i = 0; i < ind; ++i)
     eina_strbuf_append_char(buf, ' ');
   return ind;
}

#define DOC_LINE_LIMIT 79
#define DOC_LINE_TEST 59
#define DOC_LINE_OVER 39

#define DOC_LIMIT(ind) ((ind > DOC_LINE_TEST) ? (ind + DOC_LINE_OVER) \
                                              : DOC_LINE_LIMIT)

static void
_generate_ref(const Eolian_Unit *src, const char *refn, Eina_Strbuf *wbuf,
              Eina_Bool use_legacy)
{
   const Eolian_Declaration *decl = eolian_declaration_get_by_name(refn);
   if (decl)
     {
        char *n = strdup(eolian_declaration_name_get(decl));
        char *p = n;
        while ((p = strchr(p, '.'))) *p = '_';
        eina_strbuf_append(wbuf, n);
        free(n);
        return;
     }

   /* not a plain declaration, so it must be struct/enum field or func */
   const char *sfx = strrchr(refn, '.');
   if (!sfx) goto noref;

   Eina_Stringshare *bname = eina_stringshare_add_length(refn, sfx - refn);

   const Eolian_Typedecl *tp = eolian_typedecl_struct_get_by_name(src, bname);
   if (tp)
     {
        if (!eolian_typedecl_struct_field_get(tp, sfx + 1))
          {
             eina_stringshare_del(bname);
             goto noref;
          }
        _generate_ref(src, bname, wbuf, use_legacy);
        eina_strbuf_append(wbuf, sfx);
        eina_stringshare_del(bname);
        return;
     }

   tp = eolian_typedecl_enum_get_by_name(src, bname);
   if (tp)
     {
        const Eolian_Enum_Type_Field *efl = eolian_typedecl_enum_field_get(tp, sfx + 1);
        if (!efl)
          {
             eina_stringshare_del(bname);
             goto noref;
          }
        Eina_Stringshare *str = eolian_typedecl_enum_field_c_name_get(efl);
        eina_strbuf_append(wbuf, str);
        eina_stringshare_del(bname);
        return;
     }

   const Eolian_Class *cl = eolian_class_get_by_name(src, bname);
   const Eolian_Function *fn = NULL;
   /* match methods and properties; we're only figuring out existence */
   Eolian_Function_Type ftype = EOLIAN_UNRESOLVED;
   if (!cl)
     {
        const char *mname;
        if (!strcmp(sfx, ".get")) ftype = EOLIAN_PROP_GET;
        else if (!strcmp(sfx, ".set")) ftype = EOLIAN_PROP_SET;
        if (ftype != EOLIAN_UNRESOLVED)
          {
             eina_stringshare_del(bname);
             mname = sfx - 1;
             while ((mname != refn) && (*mname != '.')) --mname;
             if (mname == refn) goto noref;
             bname = eina_stringshare_add_length(refn, mname - refn);
             cl = eolian_class_get_by_name(src, bname);
             eina_stringshare_del(bname);
          }
        if (cl)
          {
             char *meth = strndup(mname + 1, sfx - mname - 1);
             fn = eolian_class_function_get_by_name(cl, meth, ftype);
             if (ftype == EOLIAN_UNRESOLVED)
               ftype = eolian_function_type_get(fn);
             free(meth);
          }
     }
   else
     {
        fn = eolian_class_function_get_by_name(cl, sfx + 1, ftype);
        ftype = eolian_function_type_get(fn);
     }

   if (!fn) goto noref;

   Eina_Stringshare *fcn = eolian_function_full_c_name_get(fn, ftype, use_legacy);
   if (!fcn) goto noref;
   eina_strbuf_append(wbuf, fcn);
   eina_stringshare_del(fcn);
   return;
noref:
   eina_strbuf_append(wbuf, refn);
}

static int
_append_section(const Eolian_Unit *src, const char *desc, int ind, int curl,
                Eina_Strbuf *buf, Eina_Strbuf *wbuf, Eina_Bool use_legacy)
{
   Eina_Bool try_note = EINA_TRUE;
   while (*desc)
     {
        eina_strbuf_reset(wbuf);
        while (*desc && isspace(*desc) && (*desc != '\n'))
          eina_strbuf_append_char(wbuf, *desc++);
        if (try_note)
          {
#define CHECK_NOTE(str) !strncmp(desc, str ": ", sizeof(str ":"))
             if (CHECK_NOTE("Note"))
               {
                  eina_strbuf_append(wbuf, "@note ");
                  desc += sizeof("Note:");
               }
             else if (CHECK_NOTE("Warning"))
               {
                  eina_strbuf_append(wbuf, "@warning ");
                  desc += sizeof("Warning:");
               }
             else if (CHECK_NOTE("Remark"))
               {
                  eina_strbuf_append(wbuf, "@remark ");
                  desc += sizeof("Remark:");
               }
             else if (CHECK_NOTE("TODO"))
               {
                  eina_strbuf_append(wbuf, "@todo ");
                  desc += sizeof("TODO:");
               }
#undef CHECK_NOTE
             try_note = EINA_FALSE;
          }
        if (*desc == '\\')
          {
             desc++;
             if ((*desc != '@') && (*desc != '$'))
               eina_strbuf_append_char(wbuf, '\\');
             eina_strbuf_append_char(wbuf, *desc++);
          }
        else if (*desc == '@')
          {
             const char *ref = ++desc;
             if (isalpha(*desc) || (*desc == '_'))
               {
                  eina_strbuf_append(wbuf, "@ref ");
                  while (isalnum(*desc) || (*desc == '.') || (*desc == '_'))
                    ++desc;
                  if (*(desc - 1) == '.') --desc;
                  Eina_Stringshare *refn = eina_stringshare_add_length(ref, desc - ref);
                  _generate_ref(src, refn, wbuf, use_legacy);
                  eina_stringshare_del(refn);
               }
             else
               eina_strbuf_append_char(wbuf, '@');
          }
        else if (*desc == '$')
          {
             desc++;
             if (isalpha(*desc))
               eina_strbuf_append(wbuf, "@c ");
             else
               eina_strbuf_append_char(wbuf, '$');
          }
        while (*desc && !isspace(*desc))
          eina_strbuf_append_char(wbuf, *desc++);
        int limit = DOC_LIMIT(ind);
        int wlen = eina_strbuf_length_get(wbuf);
        if ((int)(curl + wlen) > limit)
          {
             curl = 3;
             eina_strbuf_append_char(buf, '\n');
             curl += _indent_line(buf, ind);
             eina_strbuf_append(buf, " * ");
             if (*eina_strbuf_string_get(wbuf) == ' ')
               eina_strbuf_remove(wbuf, 0, 1);
          }
        curl += eina_strbuf_length_get(wbuf);
        eina_strbuf_append(buf, eina_strbuf_string_get(wbuf));
        if (*desc == '\n')
          {
             desc++;
             eina_strbuf_append_char(buf, '\n');
             while (*desc == '\n')
               {
                  _indent_line(buf, ind);
                  eina_strbuf_append(buf, " *\n");
                  desc++;
                  try_note = EINA_TRUE;
               }
             curl = _indent_line(buf, ind) + 3;
             eina_strbuf_append(buf, " * ");
          }
     }
   return curl;
}

static int
_append_since(const char *since, int indent, int curl, Eina_Strbuf *buf)
{
   if (since)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
        eina_strbuf_append(buf, " *\n");
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @since ");
        eina_strbuf_append(buf, since);
        curl += strlen(since) + sizeof(" * @since ") - 1;
     }
   return curl;
}

static int
_append_extra(const char *el, int indent, int curl, Eina_Bool nl, Eina_Strbuf *buf)
{
   if (el)
     {
        eina_strbuf_append_char(buf, '\n');
        if (nl)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        eina_strbuf_append(buf, el);
        curl += strlen(el) + sizeof(" * ") - 1;
     }
   return curl;
}

static char *
_sanitize_group(const char *group)
{
   if (!group) return NULL;
   char *ret = strdup(group);
   char *p;
   while ((p = strchr(ret, '.'))) *p = '_';
   return ret;
}

static void
_append_group(Eina_Strbuf *buf, char *sgrp, int indent)
{
   if (!sgrp) return;
   eina_strbuf_append(buf, " * @ingroup ");
   eina_strbuf_append(buf, sgrp);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   free(sgrp);
}

static void
_gen_doc_brief(const Eolian_Unit *src, const char *summary, const char *since,
               const char *group, const char *el, int indent, Eina_Strbuf *buf,
               Eina_Bool use_legacy)
{
   int curl = 4 + indent;
   Eina_Strbuf *wbuf = eina_strbuf_new();
   if (indent)
     eina_strbuf_append(buf, "/**< ");
   else
     eina_strbuf_append(buf, "/** ");
   curl = _append_section(src, summary, indent, curl, buf, wbuf, use_legacy);
   eina_strbuf_free(wbuf);
   curl = _append_extra(el, indent, curl, EINA_FALSE, buf);
   curl = _append_since(since, indent, curl, buf);
   char *sgrp = _sanitize_group(group);
   if (((curl + 3) > DOC_LIMIT(indent)) || sgrp)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
        if (sgrp) eina_strbuf_append(buf, " *");
     }
   if (sgrp)
     {
        eina_strbuf_append_char(buf, '\n');
        _indent_line(buf, indent);
     }
   _append_group(buf, sgrp, indent);
   eina_strbuf_append(buf, " */");
}

static void
_gen_doc_full(const Eolian_Unit *src, const char *summary,
              const char *description, const char *since,
              const char *group, const char *el, int indent, Eina_Strbuf *buf,
              Eina_Bool use_legacy)
{
   int curl = 0;
   Eina_Strbuf *wbuf = eina_strbuf_new();
   if (indent)
     eina_strbuf_append(buf, "/**<\n");
   else
     eina_strbuf_append(buf, "/**\n");
   curl += _indent_line(buf, indent);
   eina_strbuf_append(buf, " * @brief ");
   curl += sizeof(" * @brief ") - 1;
   _append_section(src, summary, indent, curl, buf, wbuf, use_legacy);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   eina_strbuf_append(buf, " *\n");
   curl = _indent_line(buf, indent);
   eina_strbuf_append(buf, " * ");
   _append_section(src, description, indent, curl + 3, buf, wbuf, use_legacy);
   curl = _append_extra(el, indent, curl, EINA_TRUE, buf);
   curl = _append_since(since, indent, curl, buf);
   eina_strbuf_append_char(buf, '\n');
   _indent_line(buf, indent);
   char *sgrp = _sanitize_group(group);
   if (sgrp)
     {
        eina_strbuf_append(buf, " *\n");
        _indent_line(buf, indent);
     }
   _append_group(buf, sgrp, indent);
   eina_strbuf_append(buf, " */");
   eina_strbuf_free(wbuf);
}

static Eina_Strbuf *
_gen_doc_buf(const Eolian_Unit *src, const Eolian_Documentation *doc,
             const char *group, const char *el, int indent,
             Eina_Bool use_legacy)
{
   if (!doc) return NULL;

   const char *sum = eolian_documentation_summary_get(doc);
   const char *desc = eolian_documentation_description_get(doc);
   const char *since = eolian_documentation_since_get(doc);

   Eina_Strbuf *buf = eina_strbuf_new();
   if (!desc)
     _gen_doc_brief(src, sum, since, group, el, indent, buf, use_legacy);
   else
     _gen_doc_full(src, sum, desc, since, group, el, indent, buf, use_legacy);
   return buf;
}

Eina_Strbuf *
eo_gen_docs_full_gen(const Eolian_Unit *src, const Eolian_Documentation *doc,
                     const char *group, int indent, Eina_Bool use_legacy)
{
   return _gen_doc_buf(src, doc, group, NULL, indent, use_legacy);
}

Eina_Strbuf *
eo_gen_docs_event_gen(const Eolian_Unit *src, const Eolian_Event *ev,
                      const char *group)
{
   if (!ev) return NULL;

   const Eolian_Documentation *doc = eolian_event_documentation_get(ev);

   char buf[1024];
   const Eolian_Type *rt = eolian_event_type_get(ev);
   const char *p = NULL;
   if (rt)
     {
        p = buf;
        Eina_Stringshare *rts = eolian_type_c_type_get(rt);
        snprintf(buf, sizeof(buf), "@return %s", rts);
        eina_stringshare_del(rts);
     }

   if (!doc)
     {
        Eina_Strbuf *bufs = eina_strbuf_new();
        eina_strbuf_append(bufs, "/**\n * No description\n");
        if (p)
          {
             eina_strbuf_append(bufs, " * ");
             eina_strbuf_append(bufs, p);
             eina_strbuf_append_char(bufs, '\n');
          }
        eina_strbuf_append(bufs, " */");
        return bufs;
     }

   return _gen_doc_buf(src, doc, group, p, 0, EINA_FALSE);
}

Eina_Strbuf *
eo_gen_docs_func_gen(const Eolian_Unit *src, const Eolian_Function *fid,
                     Eolian_Function_Type ftype, int indent,
                     Eina_Bool use_legacy)
{
   const Eolian_Function_Parameter *par = NULL;
   const Eolian_Function_Parameter *vpar = NULL;

   const Eolian_Documentation *doc, *pdoc, *rdoc;

   Eina_Iterator *itr = NULL;
   Eina_Iterator *vitr = NULL;
   Eina_Bool force_out = EINA_FALSE;

   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Strbuf *wbuf = NULL;

   const char *sum = NULL, *desc = NULL, *since = NULL;

   int curl = 0;

   const char *group = eolian_class_full_name_get(eolian_function_class_get(fid));

   const Eolian_Implement *fimp = eolian_function_implement_get(fid);

   if (ftype == EOLIAN_METHOD)
     {
        doc = eolian_implement_documentation_get(fimp, EOLIAN_METHOD);
        pdoc = NULL;
     }
   else
     {
        doc = eolian_implement_documentation_get(fimp, EOLIAN_PROPERTY);
        pdoc = eolian_implement_documentation_get(fimp, ftype);
        if (!doc && pdoc) doc = pdoc;
        if (pdoc == doc) pdoc = NULL;
     }

   rdoc = eolian_function_return_documentation_get(fid, ftype);

   if (doc)
     {
         sum = eolian_documentation_summary_get(doc);
         desc = eolian_documentation_description_get(doc);
         since = eolian_documentation_since_get(doc);
         if (pdoc && eolian_documentation_since_get(pdoc))
           since = eolian_documentation_since_get(pdoc);
     }

   if (ftype == EOLIAN_METHOD)
     {
        itr = eolian_function_parameters_get(fid);
     }
   else
     {
        itr = eolian_property_keys_get(fid, ftype);
        vitr = eolian_property_values_get(fid, ftype);
        if (!vitr || !eina_iterator_next(vitr, (void**)&vpar))
          {
             eina_iterator_free(vitr);
             vitr = NULL;
         }
     }

   if (!itr || !eina_iterator_next(itr, (void**)&par))
     {
        eina_iterator_free(itr);
        itr = NULL;
     }

   /* when return is not set on getter, value becomes return instead of param */
   if (ftype == EOLIAN_PROP_GET && !eolian_function_return_type_get(fid, ftype))
     {
        const Eolian_Function_Parameter *rvpar = vpar;
        if (!eina_iterator_next(vitr, (void**)&vpar))
          {
             /* one value - not out param */
             eina_iterator_free(vitr);
             rdoc = rvpar ? eolian_parameter_documentation_get(rvpar) : NULL;
             vitr = NULL;
             vpar = NULL;
          }
        else
          {
             /* multiple values - always out params */
             eina_iterator_free(vitr);
             vitr = eolian_property_values_get(fid, ftype);
             if (!vitr)
               vpar = NULL;
             else if (!eina_iterator_next(vitr, (void**)&vpar))
               {
                  eina_iterator_free(vitr);
                  vitr = NULL;
                  vpar = NULL;
               }
          }
     }

   if (!par)
     {
        /* no keys, try values */
        itr = vitr;
        par = vpar;
        vitr = NULL;
        vpar = NULL;
        if (ftype == EOLIAN_PROP_GET)
          force_out = EINA_TRUE;
     }

   /* only summary, nothing else; generate standard brief doc */
   if (!desc && !par && !vpar && !rdoc && (ftype == EOLIAN_METHOD || !pdoc))
     {
        _gen_doc_brief(src, sum ? sum : "No description supplied.", since, group,
                       NULL, indent, buf, use_legacy);
        return buf;
     }

   wbuf = eina_strbuf_new();

   eina_strbuf_append(buf, "/**\n");
   curl += _indent_line(buf, indent);
   eina_strbuf_append(buf, " * @brief ");
   curl += sizeof(" * @brief ") - 1;
   _append_section(src, sum ? sum : "No description supplied.",
                   indent, curl, buf, wbuf, use_legacy);

   eina_strbuf_append_char(buf, '\n');
   if (desc || since || par || rdoc || pdoc)
     {
        _indent_line(buf, indent);
        eina_strbuf_append(buf, " *\n");
     }

   if (desc)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        _append_section(src, desc, indent, curl + 3, buf, wbuf, use_legacy);
        eina_strbuf_append_char(buf, '\n');
        if (par || rdoc || pdoc || since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   if (pdoc)
     {
        const char *pdesc = eolian_documentation_description_get(pdoc);
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * ");
        _append_section(src, eolian_documentation_summary_get(pdoc), indent,
            curl + 3, buf, wbuf, use_legacy);
        eina_strbuf_append_char(buf, '\n');
        if (pdesc)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
             curl = _indent_line(buf, indent);
             eina_strbuf_append(buf, " * ");
             _append_section(src, pdesc, indent, curl + 3, buf, wbuf, use_legacy);
             eina_strbuf_append_char(buf, '\n');
          }
        if (par || rdoc || since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   while (par)
     {
        const Eolian_Documentation *adoc = eolian_parameter_documentation_get(par);
        curl = _indent_line(buf, indent);

        Eolian_Parameter_Dir dir = EOLIAN_OUT_PARAM;
        if (!force_out)
          dir = eolian_parameter_direction_get(par);

        switch (dir)
          {
           case EOLIAN_OUT_PARAM:
             eina_strbuf_append(buf, " * @param[out] ");
             curl += sizeof(" * @param[out] ") - 1;
             break;
           case EOLIAN_INOUT_PARAM:
             eina_strbuf_append(buf, " * @param[in,out] ");
             curl += sizeof(" * @param[in,out] ") - 1;
             break;
           default:
             eina_strbuf_append(buf, " * @param[in] ");
             curl += sizeof(" * @param[in] ") - 1;
             break;
          }

        const char *nm = eolian_parameter_name_get(par);
        eina_strbuf_append(buf, nm);
        curl += strlen(nm);

        if (adoc)
          {
             eina_strbuf_append_char(buf, ' ');
             curl += 1;
             _append_section(src, eolian_documentation_summary_get(adoc),
                             indent, curl, buf, wbuf, use_legacy);
          }

        eina_strbuf_append_char(buf, '\n');
        if (!eina_iterator_next(itr, (void**)&par))
          {
             par = NULL;
             if (vpar)
               {
                  eina_iterator_free(itr);
                  itr = vitr;
                  par = vpar;
                  vitr = NULL;
                  vpar = NULL;
                  if (ftype == EOLIAN_PROP_GET)
                    force_out = EINA_TRUE;
               }
          }

        if (!par && (rdoc || since))
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }
   eina_iterator_free(itr);

   if (rdoc)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @return ");
        curl += sizeof(" * @return ") - 1;
        _append_section(src, eolian_documentation_summary_get(rdoc), indent,
            curl, buf, wbuf, use_legacy);
        eina_strbuf_append_char(buf, '\n');
        if (since)
          {
             _indent_line(buf, indent);
             eina_strbuf_append(buf, " *\n");
          }
     }

   if (since)
     {
        curl = _indent_line(buf, indent);
        eina_strbuf_append(buf, " * @since ");
        eina_strbuf_append(buf, since);
        eina_strbuf_append_char(buf, '\n');
     }

   _indent_line(buf, indent);
   eina_strbuf_append(buf, " *\n");

   _indent_line(buf, indent);
   _append_group(buf, _sanitize_group(group), indent);
   eina_strbuf_append(buf, " */");
   eina_strbuf_free(wbuf);
   return buf;
}
