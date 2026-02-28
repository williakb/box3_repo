#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp/display.h"
#include "lvgl.h"
#include "driver/gpio.h"

// Your minimal local BSP (buttons/storage/etc)
#include "bsp_board.h"

// BOX-3 managed BSP (display/touch/LVGL glue)
#include "bsp/esp-box-3.h"

// LVGL port (locking, task integration)
#include "esp_lvgl_port.h"

static const char *TAG = "BOX3_GAME";
static lv_obj_t *dot;

typedef struct {
    int x;
    int v;
} game_state_t;

static game_state_t g = { .x = 0, .v = 5 };

static void game_tick(lv_timer_t *t)
{
    static int x = 0;
    static int v = 5;

    x += v;
    if (x > 280) v = -5;
    if (x < 0)   v =  5;

    lv_obj_set_pos(dot, x, 140);
}

static void render_tick(lv_timer_t *t)
{
    lv_obj_set_pos(dot, g.x, 140);
}

static void game_task(void *arg)
{
    const TickType_t period = pdMS_TO_TICKS(16);
    TickType_t last = xTaskGetTickCount();

    while (1) {
        g.x += g.v;
        if (g.x > 280) g.v = -5;
        if (g.x < 0)   g.v =  5;

        vTaskDelayUntil(&last, period);
    }
}

static void ui_create(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "BOX-3 GAME");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); // if fails, try 24/20
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 12);

    dot = lv_obj_create(scr);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, 36, 36);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x00FFCC), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_outline_width(dot, 0, 0);
    lv_obj_set_style_shadow_width(dot, 0, 0);
    lv_obj_set_pos(dot, 0, 140);
}

void app_main(void)
{
    ESP_LOGI(TAG, "app_main enter");
    ESP_LOGI(TAG, "starting LVGL/display");
    lv_disp_t *disp = bsp_display_start();
    ESP_LOGI(TAG, "LVGL display started: %p", disp);

    // Try BSP backlight first
    esp_err_t bl = bsp_display_backlight_on();
    ESP_LOGI(TAG, "bsp_display_backlight_on() -> %s", esp_err_to_name(bl));

    // Guaranteed fallback: drive GPIO47 as plain ON
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BSP_LCD_BACKLIGHT,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io);
    gpio_set_level(BSP_LCD_BACKLIGHT, 1);   // if still dark, change to 0

    lvgl_port_lock(0);
    ui_create();
    lv_timer_create(render_tick, 16, NULL);
    lvgl_port_unlock();

    xTaskCreatePinnedToCore(game_task, "game", 4096, NULL, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TAG, "alive");
    }
}