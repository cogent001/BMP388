#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

#include "bmp3.h"
#include "bmp388_drv.h"

typedef struct {
    I2C_Dev *bus;
    uint8_t  addr;
} bmp388_intf_t;

static struct bmp3_dev  dev;
static bmp388_intf_t    intf;
static bool             isInit = false;
static int8_t           lastErr = BMP3_OK;
static float            groundRefMbar = BMP388_SEA_LEVEL_MBAR;

static BMP3_INTF_RET_TYPE bmpRead(uint8_t reg, uint8_t *data, uint32_t len, void *p)
{
    bmp388_intf_t *i = (bmp388_intf_t *)p;
    return i2cdevReadReg8(i->bus, i->addr, reg, (uint16_t)len, data) ? BMP3_INTF_RET_SUCCESS : -1;
}

static BMP3_INTF_RET_TYPE bmpWrite(uint8_t reg, const uint8_t *data, uint32_t len, void *p)
{
    bmp388_intf_t *i = (bmp388_intf_t *)p;
    return i2cdevWriteReg8(i->bus, i->addr, reg, (uint16_t)len, (uint8_t *)data) ? BMP3_INTF_RET_SUCCESS : -1;
}

static void bmpDelayUs(uint32_t us, void *p)
{
    (void)p;
    if (us >= 10000) {
        vTaskDelay(pdMS_TO_TICKS((us + 999) / 1000));
    } else {
        esp_rom_delay_us(us);
    }
}

bool bmp388InitWithConfig(I2C_Dev *bus, uint8_t addr, const bmp388_config_t *cfg)
{
    isInit = false;
    intf.bus  = bus;
    intf.addr = addr;

    i2cdevInit(bus);

    memset(&dev, 0, sizeof(dev));
    dev.intf     = BMP3_I2C_INTF;
    dev.intf_ptr = &intf;
    dev.read     = bmpRead;
    dev.write    = bmpWrite;
    dev.delay_us = bmpDelayUs;

    lastErr = bmp3_init(&dev);
    if (lastErr != BMP3_OK) {
        return false;
    }

    struct bmp3_settings s;
    memset(&s, 0, sizeof(s));
    s.int_settings.drdy_en     = BMP3_ENABLE;
    s.press_en                 = BMP3_ENABLE;
    s.temp_en                  = BMP3_ENABLE;
    s.odr_filter.press_os      = cfg->press_os;
    s.odr_filter.temp_os       = cfg->temp_os;
    s.odr_filter.iir_filter    = cfg->iir_filter;
    s.odr_filter.odr           = cfg->odr;

    uint32_t sel = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS | BMP3_SEL_TEMP_OS |
                   BMP3_SEL_IIR_FILTER | BMP3_SEL_ODR | BMP3_SEL_DRDY_EN;
    lastErr = bmp3_set_sensor_settings(sel, &s, &dev);
    if (lastErr < 0) {
        return false;
    }

    s.op_mode = BMP3_MODE_NORMAL;
    lastErr = bmp3_set_op_mode(&s, &dev);
    if (lastErr < 0) {
        return false;
    }

    isInit = true;
    return true;
}

bool bmp388Init(I2C_Dev *bus, uint8_t addr)
{
    const bmp388_config_t cfg = BMP388_CONFIG_DEFAULT;
    return bmp388InitWithConfig(bus, addr, &cfg);
}

bool bmp388DataReady(void)
{
    if (!isInit) {
        return false;
    }
    struct bmp3_status st;
    lastErr = bmp3_get_status(&st, &dev);
    return (lastErr == BMP3_OK) && st.intr.drdy;
}

float bmp388PressureToAltitude(float pressure_mbar, float ref_mbar)
{
    return 44330.0f * (1.0f - powf(pressure_mbar / ref_mbar, 0.190295f));
}

bool bmp388Read(bmp388_data_t *out)
{
    if (!isInit) {
        return false;
    }
    struct bmp3_data d;
    lastErr = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &d, &dev);
    if (lastErr < 0) {
        return false;
    }
    out->pressure    = (float)(d.pressure / 100.0);   /* Pa -> mbar */
    out->temperature = (float)d.temperature;
    out->asl         = bmp388PressureToAltitude(out->pressure, BMP388_SEA_LEVEL_MBAR);
    out->altitude    = bmp388PressureToAltitude(out->pressure, groundRefMbar);
    return true;
}

bool bmp388Test(void)
{
    if (!isInit) {
        return false;
    }
    bmp388_data_t d;
    if (!bmp388Read(&d)) {
        return false;
    }
    return d.pressure > 300.0f && d.pressure < 1250.0f;
}

void bmp388SetGroundReference(float pressure_mbar)
{
    groundRefMbar = pressure_mbar;
}

bool bmp388CalibrateGround(uint16_t samples)
{
    if (!isInit || samples == 0) {
        return false;
    }
    double sum = 0;
    uint16_t n = 0;
    uint32_t guard = 0;
    while (n < samples && guard++ < (uint32_t)samples * 100u) {
        if (bmp388DataReady()) {
            bmp388_data_t d;
            if (bmp388Read(&d)) {
                sum += d.pressure;
                n++;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    if (n < samples) {
        return false;
    }
    groundRefMbar = (float)(sum / n);
    return true;
}

int8_t bmp388GetLastError(void)
{
    return lastErr;
}
