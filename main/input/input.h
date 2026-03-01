#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Virtual joystick from touch drag
    float joy_x;          // -1..1
    float joy_y;          // -1..1
    bool  joy_active;     // touch currently controlling joystick

    // Action button (front button)
    bool  action_down;       // level
    bool  action_pressed;    // rising edge (one-tick pulse)
    bool  action_released;   // falling edge (one-tick pulse)

    // Optional raw touch info
    bool  touch_down;
    int16_t touch_x;
    int16_t touch_y;
} input_state_t;

// Call once after BSP/LVGL is up.
void input_init(void);

// Call every game tick.
void input_update(uint32_t dt_ms);

// Read latest snapshot (copy).
input_state_t input_get_state(void);

#ifdef __cplusplus
}
#endif