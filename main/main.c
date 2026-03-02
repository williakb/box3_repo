#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/esp-box-3.h"

#include "game.h"
#include "audio.h"
#include "imu.h"

static void imu_task(void *arg);

void app_main(void)
{
    bsp_display_start();
    bsp_display_brightness_set(100);

    audio_init();
    audio_beep(880, 100, 90);

    imu_init();

    xTaskCreatePinnedToCore(imu_task, "imu_task", 4096, NULL, 5, NULL, 1);

    game_start();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void imu_task(void *arg)
{
    (void)arg;
    while (1) {
        imu_poll_once();
        vTaskDelay(pdMS_TO_TICKS(10));   // 100 Hz poll
    }
}