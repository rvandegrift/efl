#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif
#define EO_BETA_API
#include <Elementary.h>

// 1. Label-equivalent setup

static void
_apply_style(Eo *obj, size_t start_pos, size_t end_pos, const char *style)
{
   Efl_Text_Cursor_Cursor *start, *end;

   start = efl_text_cursor_new(obj);
   end = efl_text_cursor_new(obj);

   efl_text_cursor_position_set(obj, start, start_pos);
   efl_text_cursor_position_set(obj, end, end_pos);

   efl_text_annotation_insert(obj, start, end, style);

   efl_text_cursor_free(obj, start);
   efl_text_cursor_free(obj, end);
}

static Eo *
_create_label(Eo *win, Eo *bx)
{
   Eo *en;
   en = efl_add(EFL_UI_TEXT_CLASS, win);
   printf("Added Efl.Ui.Text object\n");
   efl_ui_text_interactive_editable_set(en, EINA_FALSE);

   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(en);
   elm_box_pack_end(bx, en);
   return en;
}

void
test_efl_ui_text_label(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx;
   Eo *en;

   win = elm_win_util_standard_add("label", "Label");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   en = _create_label(win, bx);
   efl_text_set(en, "This is a\t small label");
   //                012345678901234567890
   _apply_style(en, 0, 21, "font_size=12 font_weight=bold");
   efl_text_halign_set(en, EFL_TEXT_HORIZONTAL_ALIGNMENT_CENTER);
   efl_text_font_weight_set(en, EFL_TEXT_FONT_WEIGHT_BOLD);

   en = _create_label(win, bx);
   efl_text_halign_set(en, EFL_TEXT_HORIZONTAL_ALIGNMENT_CENTER);
   efl_text_set(en, "This is a text. Is also has\n"
         "newlines. There are several styles applied.");
   _apply_style(en, 40, 45, "font_weight=bold color=#ff0");
   _apply_style(en, 52, 58, "font_weight=italic color=#f00");
   efl_text_multiline_set(en, EINA_TRUE);

   en = _create_label(win, bx);
   efl_text_halign_set(en, EFL_TEXT_HORIZONTAL_ALIGNMENT_CENTER);
   efl_text_set(en, "By default 'multiline' is disabled.\n"
         "So, \\n would only work if you enable it.");

   en = _create_label(win, bx);
   efl_text_set(en, "You can input text here.");
   efl_text_font_set(en, "Sans", 14);
   efl_ui_text_interactive_editable_set(en, EINA_TRUE);
   efl_text_wrap_set(en, EFL_TEXT_FORMAT_WRAP_WORD);
   efl_ui_text_scrollable_set(en, EINA_TRUE);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, 0.5);

   en = _create_label(win, bx);
   efl_text_set(en, "Input multiline here.");
   efl_text_font_set(en, "Sans", 14);
   efl_ui_text_interactive_editable_set(en, EINA_TRUE);
   efl_text_wrap_set(en, EFL_TEXT_FORMAT_WRAP_WORD);
   efl_text_multiline_set(en, EINA_TRUE);
   efl_ui_text_scrollable_set(en, EINA_TRUE);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, 0.5);

   evas_object_resize(win, 480, 320);
   evas_object_show(win);
}

// 2. Entry-equivalent setup

typedef struct
{
   const char *wrap_mode[4];
   size_t cur_wrap;
} Test_Data;

static void
my_efl_ui_text_bt_3(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Efl_Text_Cursor_Cursor *sel_start, *sel_end;

   efl_ui_text_interactive_selection_cursors_get(data, &sel_start, &sel_end);
   const char *s = efl_canvas_text_range_text_get(data, sel_start, sel_end);

   printf("SELECTION REGION: %d - %d\n",
         efl_text_cursor_position_get(obj, sel_start),
         efl_text_cursor_position_get(obj, sel_end));
   printf("SELECTION:\n");
   if (s) printf("%s\n", s);
}

static void
my_efl_ui_text_bt_4(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *en = data;
   efl_text_cursor_object_item_insert(en, efl_text_cursor_get(en, EFL_TEXT_CURSOR_GET_MAIN),
         "size=32x32 href=emoticon");
}

static void
my_efl_ui_text_bt_6(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Eo *text_obj = data;

   Efl_Text_Format_Wrap wrap = efl_text_wrap_get(text_obj);

   switch(wrap)
     {
      case EFL_TEXT_FORMAT_WRAP_NONE:
         wrap = EFL_TEXT_FORMAT_WRAP_CHAR;
         break;
      case EFL_TEXT_FORMAT_WRAP_CHAR:
         wrap = EFL_TEXT_FORMAT_WRAP_WORD;
         break;
      case EFL_TEXT_FORMAT_WRAP_WORD:
         wrap = EFL_TEXT_FORMAT_WRAP_MIXED;
         break;
      case EFL_TEXT_FORMAT_WRAP_MIXED:
         wrap = EFL_TEXT_FORMAT_WRAP_NONE;
         break;
      default:
         break;
     }
   efl_text_wrap_set(text_obj, wrap);
}

static void
my_efl_ui_text_bt_5(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *en = data;
   efl_ui_text_scrollable_set(en, !efl_ui_text_scrollable_get(en));
}

void
test_efl_ui_text(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *win, *bx, *bx2, *bx3, *bt, *en;
   Efl_Text_Cursor_Cursor *main_cur, *cur;

   win = elm_win_util_standard_add("entry", "Entry");
   elm_win_autodel_set(win, EINA_TRUE);

   bx = elm_box_add(win);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_win_resize_object_add(win, bx);
   evas_object_show(bx);

   en = efl_add(EFL_UI_TEXT_CLASS, win,
         efl_text_multiline_set(efl_added, EINA_TRUE));

   printf("Added Efl.Ui.Text object\n");
   efl_key_data_set(en, "wrap_idx", 0);
   efl_text_set(en, "Hello world! Goodbye world! This is a test text for the"
         " new UI Text widget.\xE2\x80\xA9This is the next paragraph.\nThis"
         " is the next line.\nThis is Yet another line! Line and paragraph"
         " separators are actually different!");
   efl_text_font_set(en, "Sans", 14);
   efl_text_font_weight_set(en, EFL_TEXT_FONT_WEIGHT_BOLD);
   efl_text_font_slant_set(en, EFL_TEXT_FONT_SLANT_ITALIC);
   efl_text_font_width_set(en, EFL_TEXT_FONT_WIDTH_ULTRACONDENSED);
   efl_text_normal_color_set(en, 255, 255, 255, 255);

   main_cur = efl_text_cursor_get(en, EFL_TEXT_CURSOR_GET_MAIN);
   cur = efl_text_cursor_new(en);

   efl_text_cursor_position_set(en, cur, 2);
   efl_text_cursor_object_item_insert(en, cur, "size=32x32 href=emoticon");
   efl_text_cursor_position_set(en, cur, 50);
   efl_text_cursor_object_item_insert(en, cur, "size=32x32 href=emoticon");

   efl_text_cursor_position_set(en, main_cur, 5);
   efl_text_cursor_position_set(en, cur, 20);

   efl_text_annotation_insert(en, main_cur, cur, "a href=#hello");

   efl_ui_text_interactive_editable_set(en, EINA_TRUE);
   efl_ui_text_scrollable_set(en, EINA_TRUE);
   evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(en, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, en);
   evas_object_show(en);
   elm_object_focus_set(en, EINA_TRUE);

   bx2 = elm_box_add(win);
   elm_box_horizontal_set(bx2, EINA_TRUE);
   evas_object_size_hint_weight_set(bx2, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx2, EVAS_HINT_FILL, EVAS_HINT_FILL);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Sel");
   evas_object_smart_callback_add(bt, "clicked", my_efl_ui_text_bt_3, en);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   elm_object_focus_allow_set(bt, EINA_FALSE);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Ins");
   evas_object_smart_callback_add(bt, "clicked", my_efl_ui_text_bt_4, en);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   elm_object_focus_allow_set(bt, EINA_FALSE);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Scrl");
   evas_object_smart_callback_add(bt, "clicked", my_efl_ui_text_bt_5, en);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   elm_object_focus_allow_set(bt, EINA_FALSE);
   evas_object_show(bt);

   bt = elm_button_add(win);
   elm_object_text_set(bt, "Wr");
   evas_object_smart_callback_add(bt, "clicked", my_efl_ui_text_bt_6, en);
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(bt, EVAS_HINT_EXPAND, 0.0);
   elm_box_pack_end(bx2, bt);
   elm_object_focus_allow_set(bt, EINA_FALSE);
   evas_object_show(bt);

   bx3 = elm_box_add(win);
   elm_box_horizontal_set(bx3, EINA_TRUE);
   evas_object_size_hint_weight_set(bx3, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(bx3, EVAS_HINT_FILL, EVAS_HINT_FILL);

   elm_box_pack_end(bx, bx3);
   elm_box_pack_end(bx, bx2);
   evas_object_show(bx3);
   evas_object_show(bx2);

   evas_object_resize(win, 480, 320);
   evas_object_show(win);
}
