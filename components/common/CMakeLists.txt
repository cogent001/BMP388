# Minimal stand-in for ED_drone's "common" component.
# Only provides the headers that ED_drone's i2c_drv needs, so i2c_drv can be built
# unmodified in this standalone test project. NOT used when merged into ED_drone.
idf_component_register(INCLUDE_DIRS "include"
                       REQUIRES driver freertos esp_timer)
