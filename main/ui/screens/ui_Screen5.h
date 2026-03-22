#ifndef _SQUARELINE_PROJECT_UI_SCREEN5_H
#define _SQUARELINE_PROJECT_UI_SCREEN5_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// SCREEN: ui_Screen5
extern lv_obj_t *ui_Screen5;
void ui_Screen5_screen_init(void);
void ui_Screen5_screen_destroy(void);

// For setting which number (SMS or CALL) we are editing
typedef enum {
    EDIT_MODE_SMS,
    EDIT_MODE_CALL,
    EDIT_MODE_SOS
} edit_mode_t;

void ui_open_numpad(edit_mode_t mode);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
