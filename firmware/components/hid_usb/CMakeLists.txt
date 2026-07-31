idf_component_register(
    SRCS "hid_usb.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash
    PRIV_REQUIRES esp_timer driver esp_tinyusb)
