#pragma once
#include "config.h"
#include "esp_log.h"

#ifndef DEBUG_MODULE
#define DEBUG_MODULE "NULL"
#endif
#define DEBUG_PRINT(fmt, ...)  ESP_LOGD(DEBUG_MODULE, fmt, ##__VA_ARGS__)
#define DEBUG_PRINTD(fmt, ...) ESP_LOGD(DEBUG_MODULE, fmt, ##__VA_ARGS__)
#define DEBUG_PRINTI(fmt, ...) ESP_LOGI(DEBUG_MODULE, fmt, ##__VA_ARGS__)
#define DEBUG_PRINTW(fmt, ...) ESP_LOGW(DEBUG_MODULE, fmt, ##__VA_ARGS__)
#define DEBUG_PRINTE(fmt, ...) ESP_LOGE(DEBUG_MODULE, fmt, ##__VA_ARGS__)
