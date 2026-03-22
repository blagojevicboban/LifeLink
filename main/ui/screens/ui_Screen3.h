#ifndef UI_SCREEN3_H
#define UI_SCREEN3_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"

// SCREEN: ui_Screen3
extern void ui_Screen3_screen_init(void);
extern void ui_Screen3_screen_destroy(void);
extern void ui_event_Screen3(lv_event_t *e);

extern lv_obj_t *ui_Screen3;
extern lv_obj_t *ui_TextAreaPhone;
extern lv_obj_t *ui_SwitchBLE;
extern lv_obj_t *ui_SwitchWiFi;
extern lv_obj_t *ui_SwitchSMS;
extern lv_obj_t *ui_SwitchCall;
extern lv_obj_t *ui_SwitchSOS;

// Globals defined in Screen 3
extern char g_phone_number[20];

// Functions
const char *ui_get_phone_number(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
