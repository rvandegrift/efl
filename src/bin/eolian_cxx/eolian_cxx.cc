
#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

#include <string>
#include <algorithm>
#include <stdexcept>
#include <iosfwd>
#include <type_traits>
#include <cassert>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <Eolian.h>

#include <Eina.hh>
#include <Eolian_Cxx.hh>

#include "grammar/klass_def.hpp"
#include "grammar/header.hpp"
#include "grammar/impl_header.hpp"

namespace eolian_cxx {

/// Program options.
struct options_type
{
   std::vector<std::string> include_dirs;
   std::vector<std::string> in_files;
   mutable Eolian_Unit const* unit;
   std::string out_file;
   bool main_header;

   options_type() : main_header(false) {}
};

efl::eina::log_domain domain("eolian_cxx");

static bool
opts_check(eolian_cxx::options_type const& opts)
{
   if (opts.in_files.empty())
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
          << "Nothing to generate?" << std::endl;
     }
   else
     {
        return true; // valid.
     }
   return false;
}

static bool
generate(const Eolian_Class* klass, eolian_cxx::options_type const& opts)
{
   std::string header_decl_file_name = opts.out_file.empty()
     ? (::eolian_class_file_get(klass) + std::string(".hh")) : opts.out_file;

   std::string header_impl_file_name = header_decl_file_name;
   std::size_t dot_pos = header_impl_file_name.rfind(".hh");
   if (dot_pos != std::string::npos)
     header_impl_file_name.insert(dot_pos, ".impl");
   else
     header_impl_file_name.insert(header_impl_file_name.size(), ".impl");

   efl::eolian::grammar::attributes::klass_def klass_def(klass, opts.unit);
   std::vector<efl::eolian::grammar::attributes::klass_def> klasses{klass_def};
   std::vector<efl::eolian::grammar::attributes::klass_def> forward_klasses{klass_def};

   std::set<std::string> c_headers;
   std::set<std::string> cpp_headers;
   c_headers.insert(eolian_class_file_get(klass) + std::string(".h"));
        
   std::function<void(efl::eolian::grammar::attributes::type_def const&)>
     variant_function;
   auto klass_name_function
     = [&] (efl::eolian::grammar::attributes::klass_name const& name)
     {
        Eolian_Class const* klass = get_klass(name, opts.unit);
        assert(klass);
        c_headers.insert(eolian_class_file_get(klass) + std::string(".h"));
        cpp_headers.insert(eolian_class_file_get(klass) + std::string(".hh"));
        efl::eolian::grammar::attributes::klass_def cls{klass, opts.unit};
        if(std::find(forward_klasses.begin(), forward_klasses.end(), cls) == forward_klasses.end())
          forward_klasses.push_back(cls);
     };
   auto complex_function
     = [&] (efl::eolian::grammar::attributes::complex_type_def const& complex)
     {
       for(auto&& t : complex.subtypes)
         {
           variant_function(t);
         }
     };
   variant_function = [&] (efl::eolian::grammar::attributes::type_def const& type)
     {
       if(efl::eolian::grammar::attributes::klass_name const*
          name = efl::eina::get<efl::eolian::grammar::attributes::klass_name>
          (&type.original_type))
         klass_name_function(*name);
       else if(efl::eolian::grammar::attributes::complex_type_def const*
              complex = efl::eina::get<efl::eolian::grammar::attributes::complex_type_def>
               (&type.original_type))
         complex_function(*complex);
     };

   std::function<void(Eolian_Class const*)> klass_function
     = [&] (Eolian_Class const* klass)
     {
       for(efl::eina::iterator<const char> inherit_iterator ( ::eolian_class_inherits_get(klass))
             , inherit_last; inherit_iterator != inherit_last; ++inherit_iterator)
         {
           Eolian_Class const* inherit = ::eolian_class_get_by_name(opts.unit, &*inherit_iterator);
           c_headers.insert(eolian_class_file_get(inherit) + std::string(".h"));
           cpp_headers.insert(eolian_class_file_get(inherit) + std::string(".hh"));
           efl::eolian::grammar::attributes::klass_def klass{inherit, opts.unit};
           if(std::find(forward_klasses.begin(), forward_klasses.end(), klass) == forward_klasses.end())
             forward_klasses.push_back(klass);

           klass_function(inherit);
         }

       efl::eolian::grammar::attributes::klass_def klass_def(klass, opts.unit);
       for(auto&& f : klass_def.functions)
         {
           variant_function(f.return_type);
           for(auto&& p : f.parameters)
             {
               variant_function(p.type);
             }
         }
       for(auto&& e : klass_def.events)
         {
           if(e.type)
             variant_function(*e.type);
         }
     };
   klass_function(klass);
   
   cpp_headers.erase(eolian_class_file_get(klass) + std::string(".hh"));
   
   std::string guard_name;
   as_generator(*(efl::eolian::grammar::string << "_") << efl::eolian::grammar::string << "_EO_HH")
     .generate(std::back_insert_iterator<std::string>(guard_name)
               , std::make_tuple(klass_def.namespaces, klass_def.eolian_name)
               , efl::eolian::grammar::context_null{});

   std::tuple<std::string, std::set<std::string>&, std::set<std::string>&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              > attributes
   {guard_name, c_headers, cpp_headers, klasses, forward_klasses, klasses, klasses};

   if(opts.out_file == "-")
     {
        std::ostream_iterator<char> iterator(std::cout);

        efl::eolian::grammar::class_header.generate(iterator, attributes, efl::eolian::grammar::context_null());
        std::endl(std::cout);

        efl::eolian::grammar::impl_header.generate(iterator, klasses, efl::eolian::grammar::context_null());

        std::endl(std::cout);
        std::flush(std::cout);
        std::flush(std::cerr);
     }
   else
     {
        std::ofstream header_decl;
        header_decl.open(header_decl_file_name);
        if (!header_decl.good())
          {
             EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
               << "Can't open output file: " << header_decl_file_name << std::endl;
             return false;
          }

        std::ofstream header_impl;
        header_impl.open(header_impl_file_name);
        if (!header_impl.good())
          {
             EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
               << "Can't open output file: " << header_impl_file_name << std::endl;
             return false;
          }

        efl::eolian::grammar::class_header.generate
          (std::ostream_iterator<char>(header_decl), attributes, efl::eolian::grammar::context_null());

        efl::eolian::grammar::impl_header.generate
          (std::ostream_iterator<char>(header_impl), klasses, efl::eolian::grammar::context_null());

        header_impl.close();
        header_decl.close();
     }
   return true;
}

static void
run(options_type const& opts)
{
   if(!opts.main_header)
     {
       const Eolian_Class *klass = NULL;
       char* dup = strdup(opts.in_files[0].c_str());
       char* base = basename(dup);
       klass = ::eolian_class_get_by_file(NULL, base);
       free(dup);
       if (klass)
         {
           if (!generate(klass, opts))
             {
               EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
                 << "Error generating: " << ::eolian_class_name_get(klass)
                 << std::endl;
               assert(false && "error generating class");
             }
         }
       else
         {
           std::abort();
         }
     }
   else
     {
       std::set<std::string> headers;
       std::set<std::string> eo_files;

       for(auto&& name : opts.in_files)
         {
           Eolian_Unit const* unit = ::eolian_file_parse(name.c_str());
           if(!unit)
             {
               EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
                 << "Failed parsing: " << name << ".";
             }
           else
             {
               if(!opts.unit)
                 opts.unit = unit;
             }
           char* dup = strdup(name.c_str());
           char* base = basename(dup);
           Eolian_Class const* klass = ::eolian_class_get_by_file(unit, base);
           free(dup);
           if (klass)
             {
               std::string filename = eolian_class_file_get(klass);
               headers.insert(filename + std::string(".hh"));
               eo_files.insert(filename);
             }
         }

       using efl::eolian::grammar::header_include_directive;
       using efl::eolian::grammar::implementation_include_directive;

       auto main_header_grammar =
         *header_include_directive // sequence<string>
         << *implementation_include_directive // sequence<string>
         ;

       std::tuple<std::set<std::string>&, std::set<std::string>&> attributes{headers, eo_files};

       std::ofstream main_header;
       main_header.open(opts.out_file);
       if (!main_header.good())
         {
           EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
             << "Can't open output file: " << opts.out_file << std::endl;
           return;
         }
       
       main_header_grammar.generate(std::ostream_iterator<char>(main_header)
                                    , attributes, efl::eolian::grammar::context_null());
     }
}

static void
database_load(options_type const& opts)
{
   for (auto src : opts.include_dirs)
     {
        if (!::eolian_directory_scan(src.c_str()))
          {
             EINA_CXX_DOM_LOG_WARN(eolian_cxx::domain)
               << "Couldn't load eolian from '" << src << "'.";
          }
     }
   if (!::eolian_all_eot_files_parse())
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
          << "Eolian failed parsing eot files";
        assert(false && "Error parsing eot files");
     }
   if (opts.in_files.empty())
     {
       EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
         << "No input file.";
       assert(false && "Error parsing input file");
     }
   if (!opts.main_header && !::eolian_file_parse(opts.in_files[0].c_str()))
     {
       EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
         << "Failed parsing: " << opts.in_files[0] << ".";
       assert(false && "Error parsing input file");
     }
   if (!::eolian_database_validate())
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
          << "Eolian failed validating database.";
        assert(false && "Error validating database");
     }
}

} // namespace eolian_cxx {

static void
_print_version()
{
   std::cerr
     << "Eolian C++ Binding Generator (EFL "
     << PACKAGE_VERSION << ")" << std::endl;
}

static void
_usage(const char *progname)
{
   std::cerr
     << progname
     << " [options] [file.eo]" << std::endl
     << " A single input file must be provided (unless -a is specified)." << std::endl
     << "Options:" << std::endl
     << "  -a, --all               Generate bindings for all Eo classes." << std::endl
     << "  -c, --class <name>      The Eo class name to generate code for." << std::endl
     << "  -D, --out-dir <dir>     Output directory where generated code will be written." << std::endl
     << "  -I, --in <file/dir>     The source containing the .eo descriptions." << std::endl
     << "  -o, --out-file <file>   The output file name. [default: <classname>.eo.hh]" << std::endl
     << "  -n, --namespace <ns>    Wrap generated code in a namespace. [Eg: efl::ecore::file]" << std::endl
     << "  -r, --recurse           Recurse input directories loading .eo files." << std::endl
     << "  -v, --version           Print the version." << std::endl
     << "  -h, --help              Print this help." << std::endl;
   exit(EXIT_FAILURE);
}

static void
_assert_not_dup(std::string option, std::string value)
{
   if (value != "")
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain) <<
          "Option -" + option + " already set (" + value + ")";
     }
}

static eolian_cxx::options_type
opts_get(int argc, char **argv)
{
   eolian_cxx::options_type opts;

   const struct option long_options[] =
     {
       { "in",          required_argument, 0,  'I' },
       { "out-file",    required_argument, 0,  'o' },
       { "version",     no_argument,       0,  'v' },
       { "help",        no_argument,       0,  'h' },
       { "main-header", no_argument,       0,  'm' },
       { 0,             0,                 0,   0  }
     };
   const char* options = "I:D:o:c::marvh";

   int c, idx;
   while ( (c = getopt_long(argc, argv, options, long_options, &idx)) != -1)
     {
        if (c == 'I')
          {
             opts.include_dirs.push_back(optarg);
          }
        else if (c == 'o')
          {
             _assert_not_dup("o", opts.out_file);
             opts.out_file = optarg;
          }
        else if (c == 'h')
          {
             _usage(argv[0]);
          }
        else if(c == 'm')
          {
            opts.main_header = true;
          }
        else if (c == 'v')
          {
             _print_version();
             if (argc == 2) exit(EXIT_SUCCESS);
          }
     }
   if (optind != argc)
     {
       for(int i = optind; i != argc; ++i)
         opts.in_files.push_back(argv[i]);
     }

   if (!eolian_cxx::opts_check(opts))
     {
        _usage(argv[0]);
        assert(false && "Wrong options passed in command-line");
     }

   return opts;
}

int main(int argc, char **argv)
{
   try
     {
        efl::eina::eina_init eina_init;
        efl::eolian::eolian_init eolian_init;
        eolian_cxx::options_type opts = opts_get(argc, argv);
        eolian_cxx::database_load(opts);
        eolian_cxx::run(opts);
     }
   catch(std::exception const& e)
     {
       std::cerr << "EOLCXX: Eolian C++ failed generation for the following reason: " << e.what() << std::endl;
       std::cout << "EOLCXX: Eolian C++ failed generation for the following reason: " << e.what() << std::endl;
       return -1;
     }
   return 0;
}
