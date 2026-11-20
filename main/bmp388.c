/**
 * BMP388 standalone test (I2C0 bus shared with MPU6050 in ED_drone, addr 0x76).
 *
 * Mirrors the intended ED_drone usage:
 *   - sensor runs in normal mode, ODR 25 Hz, P x8 / T x1 / IIR coeff 3 (filtering done inside the sensor)
 *   - host reads one burst every 40 ms (25 Hz) with NO data-ready polling and NO INT pin
 *   - console output at 10 Hz as labeled key=value fields (latest sample + read-time / failure stats
 *     since the last print: read=avg/max [us], n=reads, fail=errors)
 *
 * Loop runs on a 20 ms base tick: read every 2nd tick (40 ms), print every 5th tick (100 ms).
 */
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "i2cdev.h"
#include "bmp388_drv.h"
#include "bmp3_defs.h"

#define BMP_ADDR            BMP388_I2C_ADDR_PRIM   /* 0x76 */
#define BASE_TICK_MS        20
#define READ_EVERY_TICKS    2                      /* 40 ms -> 25 Hz (= sensor ODR) */
#define PRINT_EVERY_TICKS   5                      /* 100 ms -> 10 Hz */
#define GROUND_CAL_SAMPLES  100

void app_main(void)
{
    printf("\n=== BMP388 test: I2C0 SDA=11 SCL=10, addr 0x%02X ===\n", BMP_ADDR);

    /* P x8, T x1, IIR coeff 3, ODR 25 Hz */
    const bmp388_config_t cfg = {
        .press_os   = BMP3_OVERSAMPLING_8X,
        .temp_os    = BMP3_NO_OVERSAMPLING,
        .iir_filter = BMP3_IIR_FILTER_COEFF_3,
        .odr        = BMP3_ODR_25_HZ,
    };

    if (!bmp388InitWithConfig(I2C0_DEV, BMP_ADDR, &cfg)) {
        printf("BMP388 init [FAIL] err=%d (check wiring / pull-ups / address)\n", bmp388GetLastError());
        return;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    printf("BMP388 init [OK], self test: %s\n", bmp388Test() ? "OK" : "FAIL");

    /* Startup only: this helper polls data-ready, which is acceptable before the real-time loop. */
    printf("Calibrating ground reference (%d samples)...\n", GROUND_CAL_SAMPLES);
    if (!bmp388CalibrateGround(GROUND_CAL_SAMPLES)) {
        printf("Ground calibration [FAIL]\n");
        return;
    }

    bmp388_data_t d = {0};
    bool have = false;
    int64_t readSumUs = 0, readMaxUs = 0;
    uint32_t reads = 0, fails = 0;

    TickType_t last = xTaskGetTickCount();
    uint32_t tick = 0;

    while (1) {
        if (tick % READ_EVERY_TICKS == 0) {
            int64_t t0 = esp_timer_get_time();
            bool ok = bmp388Read(&d);
            int64_t dt = esp_timer_get_time() - t0;
            if (ok) {
                have = true;
                reads++;
                readSumUs += dt;
                if (dt > readMaxUs) readMaxUs = dt;
            } else {
                fails++;
            }
        }

        if (have && tick % PRINT_EVERY_TICKS == 0) {
            printf("t=%lldms | P=%.3fmbar T=%.2fC asl=%.3fm alt=%.3fm | read=%lld/%lldus n=%lu fail=%lu\n",
                   (long long)(esp_timer_get_time() / 1000),
                   d.pressure, d.temperature, d.asl, d.altitude,
                   (long long)(reads ? readSumUs / reads : 0), (long long)readMaxUs,
                   (unsigned long)reads, (unsigned long)fails);   /* read = avg/max [us] */
            readSumUs = readMaxUs = 0;
            reads = fails = 0;
        }

        tick++;
        vTaskDelayUntil(&last, pdMS_TO_TICKS(BASE_TICK_MS));
    }
}
