#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp/esp-box-3.h"

#include "scene.h"   // provides game_start()

void app_main(void)
{
    // Start display + LVGL port task
    bsp_display_start();
    
    // Force backlight on (BSP API)
    bsp_display_brightness_set(100);   // 0..100

    // Start game loop task (scene setup happens inside game task)
    game_start();

    // Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
