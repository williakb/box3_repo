#include "play_scene.h"
#include <stdio.h>
#include "audio.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include <math.h>
#include "imu.h"

typedef struct {
    scene_t base;          // MUST be first
    lv_obj_t* dot;
    int32_t x;
    int32_t y;
    int32_t speed_px_s;
} play_scene_t;

static void play_enter(scene_t* s);
static void play_leave(scene_t* s);
static void play_update(scene_t* s, uint32_t dt_ms, const input_state_t* in);

static play_scene_t g_play = {
    .base = {
        .scene_enter_cb  = play_enter,
        .scene_leave_cb  = play_leave,
        .scene_update_cb = play_update,
    },
    .dot = NULL,
    .x = 160,
    .y = 120,
    .speed_px_s = 180,
};

scene_t* play_scene_get(void)
{
    return &g_play.base;
}

static void play_enter(scene_t* s)
{
    play_scene_t* ps = (play_scene_t*)s;

    ps->x = 160;
    ps->y = 120;
    ps->speed_px_s = 180;

    if (!lvgl_port_lock(200)) {
        printf("LVGL LOCK FAILED in play_enter\n");
        return;
    }

    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

    // Create once
    if (ps->dot == NULL) {
        lv_obj_t* label = lv_label_create(scr);
        lv_label_set_text(label, "PLAY SCENE");
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

        ps->dot = lv_obj_create(scr);
        lv_obj_set_size(ps->dot, 40, 40);
        lv_obj_set_style_bg_opa(ps->dot, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(ps->dot, lv_color_white(), 0);
        lv_obj_set_style_border_width(ps->dot, 0, 0);
        lv_obj_set_pos(ps->dot, ps->x, ps->y);

        printf("Created dot=%p\n", (void*)ps->dot);
    }

    lvgl_port_unlock();
}

static void play_leave(scene_t* s)
{
    play_scene_t* ps = (play_scene_t*)s;

    if (!lvgl_port_lock(200)) return;

    if (ps->dot) {
        lv_obj_del(ps->dot);
        ps->dot = NULL;
    }

    lvgl_port_unlock();
}

static void play_update(scene_t* s, uint32_t dt_ms, const input_state_t* in)
{
    play_scene_t* ps = (play_scene_t*)s;
    const float dt = (float)dt_ms / 1000.0f;

    // 1) Get IMU tilt from cached sample (no I2C here)
    float tilt_x = 0.0f;
    float tilt_y = 0.0f;
    imu_sample_t imu = {0};

    if (imu_read_cached(&imu) && imu.valid) {
        // Axis/sign may need tuning based on orientation
        tilt_x = imu.ax_g;
        tilt_y = -imu.ay_g;

        // Clamp tilt to [-1, 1]
        if (tilt_x > 1.0f) tilt_x = 1.0f;
        if (tilt_x < -1.0f) tilt_x = -1.0f;
        if (tilt_y > 1.0f) tilt_y = 1.0f;
        if (tilt_y < -1.0f) tilt_y = -1.0f;
    }

    // 2) Choose input: touch wins when active, else tilt
    float jx = (in && in->joy_active) ? in->joy_x : tilt_x;
    float jy = (in && in->joy_active) ? in->joy_y : tilt_y;

    // 3) Deadzone
    if (fabsf(jx) < 0.10f) jx = 0.0f;
    if (fabsf(jy) < 0.10f) jy = 0.0f;

    // 4) Integrate motion
    ps->x += (int32_t)(jx * (float)ps->speed_px_s * dt);
    ps->y += (int32_t)(jy * (float)ps->speed_px_s * dt);

    // 5) Clamp to screen (adjust if your panel differs)
    const int32_t W = 320;
    const int32_t H = 240;
    const int32_t DOT = 40;

    if (ps->x < 0) ps->x = 0;
    if (ps->y < 0) ps->y = 0;
    if (ps->x > (W - DOT)) ps->x = (W - DOT);
    if (ps->y > (H - DOT)) ps->y = (H - DOT);

    // 6) Update LVGL object position
    if (ps->dot && lvgl_port_lock(10)) {
        lv_obj_set_pos(ps->dot, ps->x, ps->y);
        lvgl_port_unlock();
    }

    // 7) Optional: shake/spin beep with cooldown
    static uint32_t beep_cd_ms = 0;
    if (beep_cd_ms > dt_ms) beep_cd_ms -= dt_ms;
    else beep_cd_ms = 0;

    if (imu.valid) {
        float spin = fabsf(imu.gx_dps) + fabsf(imu.gy_dps) + fabsf(imu.gz_dps);
        if (spin > 400.0f && beep_cd_ms == 0) {
            audio_beep(1200, 60, 70);
            beep_cd_ms = 300; // ms
        }
    }
}