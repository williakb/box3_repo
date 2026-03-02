#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float ax_g, ay_g, az_g;
    float gx_dps, gy_dps, gz_dps;
    float roll_deg, pitch_deg;
    bool  valid;
} imu_sample_t;
void imu_poll_once(void);
bool imu_read_cached(imu_sample_t *out);
bool imu_init(void);
bool imu_read(imu_sample_t *out);