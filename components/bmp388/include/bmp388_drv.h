/**
 * bmp388_drv.h - BMP388 barometer driver for ED_drone (adapter over Bosch BMP3 Sensor API)
 *
 * Bosch BMP3_SensorAPI (bmp3.c / bmp3.h / bmp3_defs.h) is used unmodified.
 * This adapter binds it to ED_drone's i2cdev layer and provides:
 *   - sensor configuration (OSR / IIR / ODR)
 *   - pressure [mbar], temperature [degC], ASL [m] and ground-relative altitude [m]
 *   - data-ready polling
 *
 * ED_drone integration: copy components/bmp388 as-is; the only dependency is i2c_drv (i2cdev.h).
 * Fill baro_t in sensorsTask like:
 *     baro.pressure = d.pressure; baro.temperature = d.temperature; baro.asl = d.asl;
 */
#ifndef BMP388_DRV_H_
#define BMP388_DRV_H_

#include <stdint.h>
#include <stdbool.h>
#include "i2cdev.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BMP388_I2C_ADDR_PRIM   0x76   /* SDO = GND */
#define BMP388_I2C_ADDR_SEC    0x77   /* SDO = VDDIO */

#define BMP388_SEA_LEVEL_MBAR  1013.25f

/** Values are Bosch constants (BMP3_OVERSAMPLING_*, BMP3_IIR_FILTER_*, BMP3_ODR_*). */
typedef struct {
    uint8_t press_os;
    uint8_t temp_os;
    uint8_t iir_filter;
    uint8_t odr;
} bmp388_config_t;

/* Altitude-control oriented default: P x8, T x1, IIR coeff 3, ODR 50 Hz.
 * (Values are Bosch macro numbers: OSR_8X=3, NO_OSR=0, IIR_3=2, ODR_50=2) */
#define BMP388_CONFIG_DEFAULT { 3, 0, 2, 2 }

typedef struct {
    float pressure;      /* mbar (hPa) */
    float temperature;   /* degC */
    float asl;           /* m, relative to standard sea level 1013.25 mbar */
    float altitude;      /* m, relative to ground reference (see bmp388CalibrateGround) */
} bmp388_data_t;

/** Initialise sensor on the given bus with default config. addr: BMP388_I2C_ADDR_PRIM/SEC. */
bool bmp388Init(I2C_Dev *bus, uint8_t addr);

/** Initialise with an explicit configuration. */
bool bmp388InitWithConfig(I2C_Dev *bus, uint8_t addr, const bmp388_config_t *cfg);

/** Connection / sanity test (chip id and a plausible pressure reading). */
bool bmp388Test(void);

/** True when the sensor has a new pressure sample since the last read. */
bool bmp388DataReady(void);

/** Read the latest compensated sample. Returns false on bus/sensor error. */
bool bmp388Read(bmp388_data_t *out);

/** Set ground reference pressure [mbar] used for bmp388_data_t.altitude. */
void bmp388SetGroundReference(float pressure_mbar);

/** Average `samples` new readings (blocking) and use as ground reference. */
bool bmp388CalibrateGround(uint16_t samples);

/** Barometric formula (same as Crazyflie): altitude of `pressure` above `ref` pressure, in m. */
float bmp388PressureToAltitude(float pressure_mbar, float ref_mbar);

/** Last Bosch API result code (0 = ok) for diagnostics. */
int8_t bmp388GetLastError(void);

#ifdef __cplusplus
}
#endif

#endif /* BMP388_DRV_H_ */
