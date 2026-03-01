#include "play_scene.h"
#include <stdio.h>

#include "lvgl.h"
#include "esp_lvgl_port.h"

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

    float dt = (float)dt_ms / 1000.0f;
    ps->x += (int32_t)(in->joy_x * (float)ps->speed_px_s * dt);
    ps->y += (int32_t)(in->joy_y * (float)ps->speed_px_s * dt);

    // Clamp for 320x240
    if (ps->x < 0) ps->x = 0;
    if (ps->y < 0) ps->y = 0;
    if (ps->x > 320 - 40) ps->x = 320 - 40;
    if (ps->y > 240 - 40) ps->y = 240 - 40;

    if (in->action_pressed) {
        ps->x = 160;
        ps->y = 120;
    }

    if (!ps->dot) return;

    if (lvgl_port_lock(10)) {
        lv_obj_set_pos(ps->dot, ps->x, ps->y);
        lvgl_port_unlock();
    }
}
