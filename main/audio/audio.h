#pragma once
#include <stdint.h>
#include <stdbool.h>

void audio_init(void);
void audio_beep(uint16_t freq_hz, uint16_t ms, int volume_percent);