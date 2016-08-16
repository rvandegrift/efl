/* EINA - EFL data type library
 * Copyright (C) 2015 Carsten Haitzler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#include <Eina.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// right now this is quick and dirty and may have some parsing ... frailty,
// so don't put malicious data through it... :) but cat in eina bt's through
// this to get a nicely clean and readable bt with filenames of binaries,
// shared objects, source files, and line numbers. even nicely colored and
// columnated. this is more the start of a bunch of debug tools for efl to make
// it easier to identify issues.
//
// how to use:
//
// cat mybacktrace.txt | eina_btlog
//
// (or just run it and copy & paste in on stdin - what i do mostly, and out
// pops a nice backtrace, hit ctrl+d to end)

#if defined (__MacOSX__) || (defined (__MACH__) && defined (__APPLE__))
# define ATOS_COMPATIBLE
#endif

typedef struct _Bt Bt;

struct _Bt
{
   char *bin_dir;
   char *bin_name;
   char *file_dir;
   char *file_name;
   char *func_name;
   int line;
};

typedef Eina_Bool (*Translate_Func)(const char *bin_dir,
                                    const char *bin_name,
                                    unsigned long long addr,
                                    char **file_dir,
                                    char **file_name,
                                    char **func_name,
                                    int *file_line);

typedef struct _Translation_Desc Translation_Desc;

struct _Translation_Desc
{
   const char *name;
   const char *test;
   Translate_Func func;
};

static Translate_Func _translate = NULL;
static Eina_Bool color = EINA_TRUE;

static void
path_split(const char *path, char **dir, char **file)
{
   const char *p;

   if (!path)
     {
        *dir = NULL;
        *file = NULL;
        return;
     }
   p = strrchr(path, '/');
   if (!p)
     {
        *dir = NULL;
        *file = strdup(path);
        return;
     }
   *dir = malloc(p - path + 1);
   if (!*dir)
     {
        *dir = NULL;
        *file = NULL;
        return;
     }
   strncpy(*dir, path, p - path);
   (*dir)[p - path] = 0;
   *file = strdup(p + 1);
}

static Eina_Bool
_addr2line(const char *bin_dir, const char *bin_name, unsigned long long addr,
           char **file_dir, char **file_name, char **func_name, int *file_line)
{
   char buf[4096], func[4096], *f1 = NULL, *f2 = NULL;
   Eina_Bool ok = EINA_FALSE;
   int line;
   FILE *p;

   snprintf(buf, sizeof(buf), "addr2line -f -e %s/%s -C -a 0x%llx",
            bin_dir, bin_name, addr);
   p = popen(buf, "r");
   if (!p) return EINA_FALSE;
   if ((fscanf(p, "%4095s\n", buf) == 1) &&
       (fscanf(p, "%4095s\n", func) == 1))
     {
        if (fscanf(p, "%[^:]:%i\n", buf, &line) == 2)
          {
             path_split(buf, &(f1), &(f2));
             if ((!f1) || (!f2))
               {
                  free(f1);
                  free(f2);
                  pclose(p);
                  return EINA_FALSE;
               }
          }
        else
          {
             f1 = strdup("??");
             f2 = strdup("??");
          }
        *file_dir = f1;
        *file_name = f2;
        *func_name = strdup(func);
        *file_line = line;
        ok = EINA_TRUE;
     }
   pclose(p);
   return ok;
}

#ifdef ATOS_COMPATIBLE
static Eina_Bool
_atos(const char *bin_dir, const char *bin_name, unsigned long long addr,
      char **file_dir, char **file_name, char **func_name, int *file_line)
{
   char buf[4096];
   FILE *p = NULL;
   char *f1 = NULL, *s;
   Eina_Bool ret = EINA_FALSE;
   unsigned int count = 0, len;
   Eina_Bool func_done = EINA_FALSE;
   unsigned int spaces = 0, func_space_count;

   // Example of what we want to parse
   // $ atos -o /usr/local/lib/libevas.1.dylib 0xa82d
   // evas_object_clip_recalc (in libevas.1.dylib) (evas_inline.x:353)
   //
   // WARNING! Sometimes:
   // tlv_load_notification (in libdyld.dylib) + 382
   //
   // WARNING! Objective-C methods:
   // -[EcoreCocoaWindow windowDidResize:] (in libecore_cocoa.1.dylib) (ecore_cocoa_window.m:97)

   snprintf(buf, sizeof(buf), "atos -o %s/%s 0x%llx", bin_dir, bin_name, addr);
   p = popen(buf, "r");
   if (!p) goto end;

   s = fgets(buf, sizeof(buf), p);
   if (!s) goto end;

   /* Default value, used as a fallback when cannot be determined */
   *file_line = -1;

   if ((*s == '-') || (*s == '+')) /* objc methods... will contain an extra space */
     func_space_count = 2;
   else
     func_space_count = 1;

   do
     {
        if (*s == ' ') spaces++;

        if ((spaces == func_space_count) && (func_done == EINA_FALSE))
          {
             *s = '\0';
             *func_name = strndup(buf, (int)(s - &(buf[0])));
             func_done = EINA_TRUE;
          }
        else if (*s == '(')
          {
             count++;
             if ((count == 2) && (f1 == NULL))
               {
                  f1 = s + 1; /* skip the leading '(' */
               }
          }
        else if ((*s == ':') && (func_done == EINA_TRUE))
          {
             *s = '\0';
             *file_name = strndup(f1, (int)(s - f1));
             s++;
             len = strlen(s);
             s[len - 1] = '\0'; /* Remove the closing parenthesis */
             *file_line = atoi(s);
             break; /* Done */
          }
     }
   while (*(++s) != '\0');

   /* Cannot be determined */
   *file_dir = strdup("??");

   if (!*func_name) *func_name = strdup("??");
   if (!*file_name) *file_name = strdup("??");

   ret = EINA_TRUE;
end:
   if (p) pclose(p);
   return ret;
}
#endif

static Eina_List *
bt_append(Eina_List *btl, const char *btline)
{
   Bt *bt = calloc(1, sizeof(Bt));
   if (!bt) return btl;
   char *bin = strdup(btline);
   unsigned long long offset = 0, base = 0;

   // parse:
   // /usr/local/lib/libeina.so.1 0x1ec88
   // /usr/local/lib/libelementary.so.1 0x10f695
   // /usr/local/lib/libeo.so.1 0xa474
   // /usr/local/lib/libelementary.so.1 0x139bd6
   // /usr/local/bin/elementary_test 0x8196d
   // /usr/local/bin/elementary_test 0x81b6a
   if (sscanf(btline, "%s %llx %llx", bin, &offset, &base) == 3)
     {
        path_split(bin, &(bt->bin_dir), &(bt->bin_name));
        if (!bt->bin_dir) bt->bin_dir = strdup("");
        if (!bt->bin_name) bt->bin_name = strdup("");
        if (!_translate(bt->bin_dir, bt->bin_name, offset - base,
                        &(bt->file_dir), &(bt->file_name),
                        &(bt->func_name), &(bt->line)))
          {
             if (!_translate(bt->bin_dir, bt->bin_name, offset,
                             &(bt->file_dir), &(bt->file_name),
                             &(bt->func_name), &(bt->line)))
               {
                  bt->file_dir = strdup("");
                  bt->file_name = strdup("");
                  bt->func_name = strdup("");
               }
          }
        btl = eina_list_append(btl, bt);
     }
   else free(bt);
   free(bin);
   return btl;
}

static Eina_Bool
_translation_function_detect(const Translation_Desc *desc)
{
   const Translation_Desc *d = desc;
   FILE *p;
   int ret;

   while ((d->name != NULL) && (d->func != NULL) && (d->test != NULL))
     {
         p = popen(d->test, "r");
         if (p)
           {
              ret = pclose(p);
              if (ret == 0)
                {
                   _translate = d->func;
                   break;
                }
           }
         d++;
     }

   return (_translate == NULL) ? EINA_FALSE : EINA_TRUE;
}

int
main(int argc, char **argv)
{
   Eina_List *btl = NULL, *l;
   char buf[4096];
   Bt *bt;
   int cols[6] = { 0 }, len, i;
   const Translation_Desc desc[] = {
#ifdef ATOS_COMPATIBLE
        { /* Mac OS X */
           .name = "atos",
           .test = "atos --help &> /dev/null",
           .func = _atos
        },
#endif
        { /* GNU binutils */
           .name = "addr2line",
           .test = "addr2line --help &> /dev/null",
           .func = _addr2line
        },
        { /* For imported GNU binutils */
           .name = "GNU addr2line",
           .test = "gaddr2line --help &> /dev/null",
           .func = _addr2line
        },
        { NULL, NULL, NULL } /* Sentinel */
   };

   eina_init();

   for (i = 1; i < argc; i++)
     {
        if (!strcmp(argv[i], "-h"))
          {
             printf("Usage: eina_btlog [-n]\n"
                    "  -n   Do not use color escape codes\n");
             eina_shutdown();
             return 0;
          }
        else if (!strcmp(argv[i], "-n")) color = EINA_FALSE;
     }

   if (!_translation_function_detect(desc))
     {
        EINA_LOG_CRIT("Fail to determine a program to translate backtrace "
                      "into human-readable text");
        return 1;
     }

   while (fgets(buf, sizeof(buf) - 1, stdin))
     {
        btl = bt_append(btl, buf);
     }
   EINA_LIST_FOREACH(btl, l, bt)
     {
        len = strlen(bt->bin_dir);
        if (len > cols[0]) cols[0] = len;
        len = strlen(bt->bin_name);
        if (len > cols[1]) cols[1] = len;

        len = strlen(bt->file_dir);
        if (len > cols[2]) cols[2] = len;
        len = strlen(bt->file_name);
        if (len > cols[3]) cols[3] = len;

        snprintf(buf, sizeof(buf), "%i", bt->line);
        len = strlen(buf);
        if (len > cols[4]) cols[4] = len;

        len = strlen(bt->func_name);
        if (len > cols[5]) cols[5] = len;
     }
   EINA_LIST_FOREACH(btl, l, bt)
     {
        len = strlen(bt->bin_dir);
        for (i = 0; i < (cols[0] - len); i++) printf(" ");
        if (color)
          printf("\033[34m%s\033[01m\033[36m/\033[37m%s\033[0m",
                 bt->bin_dir, bt->bin_name);
        else
          printf("%s/%s",
                 bt->bin_dir, bt->bin_name);
        len = strlen(bt->bin_name);
        for (i = 0; i < (cols[1] - len); i++) printf(" ");
        printf(" | ");
        len = strlen(bt->file_dir);
        for (i = 0; i < (cols[2] - len); i++) printf(" ");
        if (color)
          printf("\033[34m%s\033[01m\033[36m/\033[37m%s\033[0m",
                 bt->file_dir, bt->file_name);
        else
          printf("%s/%s",
                 bt->file_dir, bt->file_name);
        len = strlen(bt->file_name);
        for (i = 0; i < (cols[3] - len); i++) printf(" ");

        printf(" : ");
        snprintf(buf, sizeof(buf), "%i", bt->line);
        len = strlen(buf);
        for (i = 0; i < (cols[4] - len); i++) printf(" ");
        if (color)
          {
             printf("\033[01m\033[33m%s\033[0m @ \033[32m%s\033[36m()", buf, bt->func_name);
             printf("\033[0m\n");
          }
        else
          {
             printf("%s @ %s()", buf, bt->func_name);
             printf("\n");
         }
     }
   return 0;
}
