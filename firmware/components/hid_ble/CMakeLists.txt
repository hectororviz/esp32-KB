idf_component_register(
    SRCS "hid_ble.c" "hid_ble_gap.c"
    INCLUDE_DIRS "."
    REQUIRES bt esp_hid esp_event)
