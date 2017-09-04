#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "eo_cxx_suite.h"
#include "../efl_check.h"

static const Efl_Test_Case etc[] = {
   { "Promise", eo_cxx_test_promise },
   { NULL, NULL }
};

int
main(int argc, char* argv[])
{
   int failed_count;

   if (!_efl_test_option_disp(argc, argv, etc))
     return 0;

   putenv(const_cast<char*>("EFL_RUN_IN_TREE=1"));

   failed_count = _efl_suite_build_and_run(argc - 1, (const char **)argv + 1,
                                           "Eo C++", etc);

   return (failed_count == 0) ? 0 : 255;
}
