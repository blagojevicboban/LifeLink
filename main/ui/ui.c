#include "ui.h"
#include "ui_helpers.h"

lv_obj_t *ui____initial_actions0;

void ui_init(void) {
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    ui_Screen1_screen_init();
    ui_Screen2_screen_init();
    ui_Screen3_screen_init();
    // ui_Screen4 and ui_Screen5 are initialized lazily when needed
    
    ui____initial_actions0 = lv_obj_create(NULL);
    lv_scr_load(ui_Screen1);
}
