#pragma once
#include <stdint.h>
#include "../input/input.h"   // or "input/input.h" if you fixed include dirs

typedef struct scene_t scene_t;

struct scene_t {
    void (*scene_enter_cb)(scene_t* self);
    void (*scene_leave_cb)(scene_t* self);
    void (*scene_update_cb)(scene_t* self, uint32_t dt_ms, const input_state_t* in);
};

void scene_set(scene_t* next);
void game_start(void);