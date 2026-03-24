#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ui_helpers.h"
#include "ui_events.h"

// SCREEN HEADERS
#include "screens/ui_Screen1.h"
#include "screens/ui_Screen2.h"
#include "screens/ui_Screen3.h"
#include "screens/ui_Screen4.h"
#include "screens/ui_Screen5.h"
#include "screens/ui_Screen6.h"

// GLOBAL VARIABLES
extern lv_obj_t *ui_Screen1;
extern lv_obj_t *ui_Screen2;
extern lv_obj_t *ui_Screen3;
extern lv_obj_t *ui_Screen4;
extern lv_obj_t *ui_Screen5;
extern lv_obj_t *ui_Screen6;
extern lv_obj_t *ui_LabelTime;
extern lv_obj_t *ui____initial_actions0;

// Screen 1 Elements
extern lv_obj_t *ui_LabelInfo;
extern lv_obj_t *ui_LabelPuls;
extern lv_obj_t *ui_LabelSpo;
extern lv_obj_t *ui_LabelGPS;
extern lv_obj_t *ui_LabelGPS_Icon;
extern lv_obj_t *ui_LabelGSM_Icon;
extern lv_obj_t *ui_LabelGSM_Text;
extern lv_obj_t *ui_LabelBLT;
extern lv_obj_t *ui_LabelBLE_Icon;
extern lv_obj_t *ui_LabelBatt;

// Screen 2 Elements (Debug)
extern lv_obj_t *ui_BtnDebug;
extern lv_obj_t *ui_LabelG;
extern lv_obj_t *ui_LabelX;
extern lv_obj_t *ui_LabelY;
extern lv_obj_t *ui_LabelZ;
extern lv_obj_t *ui_LabelGX;
extern lv_obj_t *ui_LabelGY;
extern lv_obj_t *ui_LabelGZ;
extern lv_obj_t *ui_LabelGSM; // GSM Label for Screen 2

// UI INIT
void ui_init(void);

// ASSETS
LV_IMG_DECLARE(ui_img_logo128_png); // assets/logo128.png
LV_IMG_DECLARE(ui_img_pulse36_png); // assets/pulse36.png
LV_IMG_DECLARE(ui_img_1716151603);  // assets/spo2-36.png

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
