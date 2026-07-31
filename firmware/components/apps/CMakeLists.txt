idf_component_register(
    SRCS "apps.c"
    INCLUDE_DIRS "."
    REQUIRES settings
    PRIV_REQUIRES keycodes display power esp_timer esp_hw_support esp_driver_tsens)
