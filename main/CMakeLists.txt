idf_component_register(SRCS "bmp388.c"
                    INCLUDE_DIRS "."
                    REQUIRES bmp388 i2c_drv esp_timer freertos)
