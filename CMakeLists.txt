cmake_minimum_required(VERSION 3.16)

# All components (bmp388, i2c_drv, common) live in ./components and are picked up automatically.
# i2c_drv is a verbatim copy of ED_drone's I2C driver (same i2cdev API, same I2C0 pins/speed as MPU6050 bus).

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(bmp388)
