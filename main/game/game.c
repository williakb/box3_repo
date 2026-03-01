#include "scene.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "input.h"
#include "play_scene.h"

static scene_t* g_scene = NULL;

void scene_set(scene_t* next)
{
    if (next == NULL) return;
    if (g_scene == next) return;

    if (g_scene && g_scene->scene_leave_cb) {
        g_scene->scene_leave_cb(g_scene);
    }

    g_scene = next;

    if (g_scene && g_scene->scene_enter_cb) {
        g_scene->scene_enter_cb(g_scene);
    }
}

static void game_task(void* arg)
{
    (void)arg;

    // Let BSP/LVGL fully come up
    vTaskDelay(pdMS_TO_TICKS(200));

    input_init();
    scene_set(play_scene_get());  // set initial scene once

    const uint32_t tick_ms = 16; // ~60Hz
    TickType_t last = xTaskGetTickCount();

    while (1) {
        TickType_t now = xTaskGetTickCount();
        uint32_t dt_ms = (uint32_t)((now - last) * portTICK_PERIOD_MS);
        last = now;

        input_update(dt_ms);
        input_state_t in = input_get_state();

        if (g_scene && g_scene->scene_update_cb) {
            g_scene->scene_update_cb(g_scene, dt_ms, &in);
        }

        vTaskDelay(pdMS_TO_TICKS(tick_ms));
    }
}

void game_start(void)
{
    (void)xTaskCreatePinnedToCore(game_task, "game", 8192, NULL, 5, NULL, 1);
}
