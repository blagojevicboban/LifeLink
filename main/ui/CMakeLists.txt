SET(SOURCES 
    screens/ui_Screen1.c
    screens/ui_Screen2.c
    screens/ui_Screen3.c
    screens/ui_Screen4.c
    screens/ui_Screen5.c
    ui.c
    ui_helpers.c
    images/ui_img_logo128_png.c
    images/ui_img_pulse36_png.c
    images/ui_img_1716151603.c
    ui_events.c)

add_library(ui ${SOURCES})
