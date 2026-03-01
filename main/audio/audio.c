#include "audio.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bsp/esp-box-3.h"
#include "esp_codec_dev.h"

#ifndef PI_F
#define PI_F 3.14159265358979323846f
#endif

static esp_codec_dev_handle_t s_codec = NULL;
static bool s_inited = false;

void audio_init(void)
{
    if (s_inited) return;

    // BSP provides this in your version
    // It initializes the speaker codec path and returns a codec handle.
    s_codec = bsp_audio_codec_speaker_init();
    if (!s_codec) {
        printf("audio_init: bsp_audio_codec_speaker_init returned NULL\n");
        return;
    }

    // Open with sample format (per your header: esp_codec_dev_sample_info_t)
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 44100,
        .channel     = 1,     // mono
        .bits_per_sample = 16,
    };

    int err = esp_codec_dev_open(s_codec, &fs);
    if (err != 0) {
        printf("audio_init: esp_codec_dev_open failed: %d\n", err);
        s_codec = NULL;
        return;
    }

    // 0..100 typically
    esp_codec_dev_set_out_vol(s_codec, 50);

    s_inited = true;
    printf("audio_init: OK\n");
}

void audio_beep(uint16_t freq_hz, uint16_t ms, int volume_percent)
{
    if (!s_inited) audio_init();
    if (!s_codec) return;

    if (volume_percent < 0) volume_percent = 0;
    if (volume_percent > 100) volume_percent = 100;
    esp_codec_dev_set_out_vol(s_codec, volume_percent);

    const int sample_rate = 44100;
    int total_samples = (sample_rate * (int)ms) / 1000;
    if (total_samples <= 0) return;

    // Small chunk buffer (stack)
    int16_t buf[256];

    // Conservative amplitude to avoid clipping
    const float amp = 0.20f * 32767.0f;
    float phase = 0.0f;
    const float phase_inc = 2.0f * PI_F * (float)freq_hz / (float)sample_rate;

    while (total_samples > 0) {
        int n = total_samples;
        if (n > (int)(sizeof(buf) / sizeof(buf[0]))) n = (int)(sizeof(buf) / sizeof(buf[0]));

        for (int i = 0; i < n; i++) {
            buf[i] = (int16_t)(sinf(phase) * amp);
            phase += phase_inc;
            if (phase > 2.0f * PI_F) phase -= 2.0f * PI_F;
        }

        // Your esp_codec_dev_write signature: (codec, void *data, int len)
        // It expects bytes.
        int bytes = n * (int)sizeof(int16_t);
        (void)esp_codec_dev_write(s_codec, (void*)buf, bytes);

        total_samples -= n;
    }
}