#include "imu.h"

#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "bsp/esp-box-3.h"
#include "icm42670.h"

static icm42670_handle_t s_imu = NULL;

static imu_sample_t s_last;
static bool s_has_last = false;

bool imu_read_cached(imu_sample_t *out)
{
    if (!out || !s_has_last) return false;
    *out = s_last;
    return true;
}

void imu_poll_once(void)
{
    imu_sample_t s;
    if (imu_read(&s) && s.valid) {
        s_last = s;
        s_has_last = true;
    }
}

bool imu_init(void)
{
    if (s_imu) return true;

    // Get BSP I2C bus (already initialized by display/touch)
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (!bus) {
        printf("imu_init: bsp_i2c_get_handle failed\n");
        return false;
    }

    // Create IMU
    esp_err_t err = icm42670_create(bus, ICM42670_I2C_ADDRESS, &s_imu);
    if (err != ESP_OK) {
        printf("imu_init: create failed: %s\n", esp_err_to_name(err));
        return false;
    }

    icm42670_cfg_t cfg = {
        .acce_fs  = ACCE_FS_4G,
        .acce_odr = ACCE_ODR_100HZ,
        .gyro_fs  = GYRO_FS_500DPS,
        .gyro_odr = GYRO_ODR_100HZ,
    };

    err = icm42670_config(s_imu, &cfg);
    if (err != ESP_OK) {
        printf("imu_init: config failed\n");
        return false;
    }

    icm42670_acce_set_pwr(s_imu, ACCE_PWR_LOWNOISE);
    icm42670_gyro_set_pwr(s_imu, GYRO_PWR_LOWNOISE);

    printf("imu_init: OK\n");
    return true;
}

bool imu_read(imu_sample_t *out)
{
    if (!out || !s_imu) return false;

    memset(out, 0, sizeof(*out));
    out->valid = false;

    icm42670_value_t a, g;

    if (icm42670_get_acce_value(s_imu, &a) != ESP_OK) return false;
    if (icm42670_get_gyro_value(s_imu, &g) != ESP_OK) return false;

    out->ax_g = a.x;
    out->ay_g = a.y;
    out->az_g = a.z;

    out->gx_dps = g.x;
    out->gy_dps = g.y;
    out->gz_dps = g.z;

    complimentary_angle_t ang;
    if (icm42670_complimentory_filter(s_imu, &a, &g, &ang) == ESP_OK) {
        out->roll_deg  = ang.roll;
        out->pitch_deg = ang.pitch;
    }

    out->valid = true;
    return true;
}