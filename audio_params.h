#pragma once
#include <QString>
#include <vector>

struct Params {
    // ... (все твои предыдущие поля остаются без изменений) ...
    float duration  = 0.5f;
    float frequency = 440.0f;
    float fm        = 0.0f;
    float noise     = 0.0f;
    float decay     = 0.3f;
    float gain      = 0.8f;
    float slide     = 0.0f;
    QString waveform = "sine";
    float attack    = 0.005f;
    float sustain   = 0.5f;
    float release   = 0.1f;
    float vibrato_rate  = 0.0f;
    float vibrato_depth = 0.0f;
    float seamless_crossfade = 0.0f;
    float arp_rate = 0.0f;
    float arp_gate = 1.0f;
    int   filter_type   = 0;
    float filter_cutoff = 20000.0f;
    float filter_resonance = 0.0f;
    int   dist_type     = 0;
    float dist_drive    = 0.0f;
    float bit_depth     = 16.0f;
    float sample_rate_div = 1.0f;
    float delay_time    = 0.0f;
    float delay_feedback= 0.0f;
    bool  ping_pong     = false;
    float reverb_mix    = 0.0f;
    float reverb_size   = 0.5f;
    float chorus_mix    = 0.0f;
    float chorus_depth  = 0.0f;
    float phaser_mix    = 0.0f;
    float tremolo_rate  = 0.0f; 
    float ring_mod_freq = 0.0f;
    bool  steps[16] = {false};
    bool  step_mode = false;
    std::vector<float> vol_env = {0.0f, 1.0f, 0.5f, 0.2f, 0.0f};
    std::vector<float> pitch_env = {0.5f, 0.5f, 0.5f};

    // НОВОЕ: Нарисованная огибающая
    std::vector<float> drawn_env;
};