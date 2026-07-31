idf_component_register(
    SRCS "app_main.c"
         "matrix.c"
         "keymap.c"
         "encoder.c"
         "hid_route.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_timer driver keycodes
    PRIV_REQUIRES hid_usb hid_ble display apps power)
