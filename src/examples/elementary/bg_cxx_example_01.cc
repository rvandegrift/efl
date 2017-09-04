#define EFL_EO_API_SUPPORT

#include <Elementary.hh>

EAPI_MAIN int
elm_main (int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_HIDDEN);

   ::efl::ui::Win win;
   // win.title_set("Bg Plain");
   win.autohide_set(true);

   win.eo_cxx::efl::Gfx::size_set(320,320);
   //win.size_set(320,320);
   win.visible_set(true);

   elm_run();
   return 0;
}
ELM_MAIN()
