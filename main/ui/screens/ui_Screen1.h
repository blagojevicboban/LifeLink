#ifndef _SQUARELINE_PROJECT_UI_SCREEN1_H
#define _SQUARELINE_PROJECT_UI_SCREEN1_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// SCREEN: ui_Screen1
extern lv_obj_t *ui_Screen1;
void ui_Screen1_screen_init(void);
void ui_Screen1_screen_destroy(void);

// UI ELEMENTS
extern lv_obj_t *ui_LabelTime;
extern lv_obj_t *ui_LabelInfo;
extern lv_obj_t *ui_LabelPuls;
extern lv_obj_t *ui_LabelSpo;
extern lv_obj_t *ui_LabelGPS;
extern lv_obj_t *ui_LabelGPS_Icon; // GPS Icon
extern lv_obj_t *ui_LabelGSM_Icon;
extern lv_obj_t *ui_LabelGSM_Text;
extern lv_obj_t *ui_LabelWiFi_Icon; // WiFi Icon
extern lv_obj_t *ui_LabelBLT;
extern lv_obj_t *ui_LabelBLE_Icon; // BLE Icon
extern lv_obj_t *ui_LabelBatt; // Battery Percentage Text

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
