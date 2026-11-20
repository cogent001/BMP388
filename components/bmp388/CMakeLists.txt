idf_component_register(SRCS "bmp3.c" "bmp388_drv.c"
                       INCLUDE_DIRS "include"
                       REQUIRES i2c_drv freertos esp_rom)
