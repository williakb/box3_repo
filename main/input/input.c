#include "input.h"
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "lvgl.h"
#include "esp_lvgl_port.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

typedef struct {
    input_state_t s;

    // Action edge detect
    bool prev_action_down;

    // Virtual joystick state
    bool joy_tracking;
    int16_t joy_center_x;
    int16_t joy_center_y;

    // Tuning
    float deadzone;       // e.g. 0.10
    float max_radius_px;  // e.g. 80 px drag = full deflection

    // LVGL capture object
    lv_obj_t* capture;

    // Lock
    portMUX_TYPE mux;
} input_ctx_t;

static input_ctx_t g = {
    .mux = portMUX_INITIALIZER_UNLOCKED
};

static float apply_deadzone(float v, float dz)
{
    float a = fabsf(v);
    if (a < dz) return 0.0f;
    float sign = (v >= 0.0f) ? 1.0f : -1.0f;
    float out = (a - dz) / (1.0f - dz);
    return sign * MIN(out, 1.0f);
}

static void update_joystick_from_touch(bool down, int16_t x, int16_t y)
{
    g.s.touch_down = down;
    g.s.touch_x = x;
    g.s.touch_y = y;

    if (!down) {
        g.joy_tracking = false;
        g.s.joy_active = false;
        g.s.joy_x = 0.0f;
        g.s.joy_y = 0.0f;
        return;
    }

    if (!g.joy_tracking) {
        g.joy_tracking = true;
        g.joy_center_x = x;
        g.joy_center_y = y;
        g.s.joy_active = true;
        g.s.joy_x = 0.0f;
        g.s.joy_y = 0.0f;
        return;
    }

    float dx = (float)(x - g.joy_center_x);
    float dy = (float)(y - g.joy_center_y);

    float nx = dx / g.max_radius_px;
    float ny = dy / g.max_radius_px;

    float mag = sqrtf(nx*nx + ny*ny);
    if (mag > 1.0f && mag > 0.0f) {
        nx /= mag;
        ny /= mag;
    }

    nx = apply_deadzone(nx, g.deadzone);
    ny = apply_deadzone(ny, g.deadzone);

    g.s.joy_active = true;
    g.s.joy_x = nx;
    g.s.joy_y = -ny;
}

// LVGL event handler runs in LVGL task context
static void capture_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        lv_point_t p;
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_indev_get_point(indev, &p);
            portENTER_CRITICAL(&g.mux);
            update_joystick_from_touch(true, (int16_t)p.x, (int16_t)p.y);
            portEXIT_CRITICAL(&g.mux);
        }
    } else if (code == LV_EVENT_RELEASED) {
        portENTER_CRITICAL(&g.mux);
        update_joystick_from_touch(false, 0, 0);
        portEXIT_CRITICAL(&g.mux);
    }
}

void input_init(void)
{
    memset(&g.s, 0, sizeof(g.s));
    g.prev_action_down = false;
    g.joy_tracking = false;
    g.deadzone = 0.10f;
    g.max_radius_px = 80.0f;

    // Create a transparent full-screen object to capture touch/drag
    if (!lvgl_port_lock(200)) return;

    lv_obj_t* scr = lv_scr_act();
    g.capture = lv_obj_create(scr);
    lv_obj_remove_style_all(g.capture);
    lv_obj_set_size(g.capture, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(g.capture, 0, 0);
    lv_obj_add_flag(g.capture, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g.capture, capture_event_cb, LV_EVENT_ALL, NULL);

    lvgl_port_unlock();
}

void input_update(uint32_t dt_ms)
{
    (void)dt_ms;

    // Clear one-tick pulses
    portENTER_CRITICAL(&g.mux);
    g.s.action_pressed = false;
    g.s.action_released = false;
    // NOTE: physical button hookup TBD (set action_down elsewhere)
    bool action_down = g.s.action_down;
    portEXIT_CRITICAL(&g.mux);

    // Edge detect based on current action_down
    portENTER_CRITICAL(&g.mux);
    if (action_down && !g.prev_action_down) g.s.action_pressed = true;
    if (!action_down && g.prev_action_down) g.s.action_released = true;
    g.prev_action_down = action_down;
    portEXIT_CRITICAL(&g.mux);
}

input_state_t input_get_state(void)
{
    input_state_t out;
    portENTER_CRITICAL(&g.mux);
    out = g.s;
    portEXIT_CRITICAL(&g.mux);
    return out;
}
