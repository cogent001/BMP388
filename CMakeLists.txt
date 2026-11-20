cmake_minimum_required(VERSION 3.16)

# Reuse ED_drone's I2C driver verbatim (same i2cdev API, same I2C0 pins/speed as MPU6050 bus).
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/../ED_drone/components/i2c_drv")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bmp388)
