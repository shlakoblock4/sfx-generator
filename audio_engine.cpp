#include "audio_engine.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <vector>

class SimpleReverb {
    std::vector<float> buf1, buf2, buf3, buf4;
    int idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;
public:
    SimpleReverb() {
        buf1.resize(1557, 0); buf2.resize(1617, 0); 
        buf3.resize(1491, 0); buf4.resize(1422, 0);
    }
    float process(float in, float feedback) {
        float out = 0;
        float y1 = buf1[idx1]; buf1[idx1] = in + y1 * feedback; out += y1; idx1 = (idx1+1)%buf1.size();
        float y2 = buf2[idx2]; buf2[idx2] = in + y2 * feedback; out += y2; idx2 = (idx2+1)%buf2.size();
        float y3 = buf3[idx3]; buf3[idx3] = in + y3 * feedback; out += y3; idx3 = (idx3+1)%buf3.size();
        float y4 = buf4[idx4]; buf4[idx4] = in + y4 * feedback; out += y4; idx4 = (idx4+1)%buf4.size();
        return out * 0.25f;
    }
};

std::vector<float> synthesize(const Params& p) {
    const int n = std::max(1, (int)(SR * p.duration));
    std::vector<float> out(n);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> noiseDist(-1.0f, 1.0f);

    float phase = 0.0f;
    float lastOutLP = 0.0f;
    float lastOutHP = 0.0f;
    float lastInHP = 0.0f;
    float hpRes = 0.0f;

    float alpha = 1.0f;
    if (p.filter_cutoff < 20000.0f && p.filter_cutoff > 20.0f) {
        float dt = 1.0f / SR;
        float rc = 1.0f / (2.0f * PI_F * p.filter_cutoff);
        alpha = dt / (rc + dt);
    }

    float steps = std::pow(2.0f, p.bit_depth) / 2.0f;
    int crushCounter = 0;
    float lastCrushed = 0.0f;

    int delaySamples = (int)(p.delay_time * SR / 1000.0f);
    std::vector<float> delayBuf(delaySamples > 0 ? delaySamples : 1, 0.0f);
    int delayIdx = 0;
    float pingPongState = 0.0f;

    SimpleReverb reverb;
    float revFeedback = 0.5f + p.reverb_size * 0.4f;

    const int chorusMax = (int)(0.05f * SR); 
    std::vector<float> chorusBuf(chorusMax, 0.0f);
    int chorusIdx = 0;

    std::vector<float> phaserBuf(4, 0.0f);

    float stepDuration = p.duration / 16.0f;

    for (int i = 0; i < n; ++i) {
        const float t = (float)i / SR;
        const float frac = (float)i / n;
        
        float curFreq = p.frequency + p.slide * frac;
        
        // МОДУЛЯЦИЯ ЧАСТОТЫ ИЗ ГРАФИЧЕСКОГО РЕДАКТОРА
        if (p.pitch_env.size() >= 2) {
            float pos = frac * (p.pitch_env.size() - 1);
            int idx = (int)pos;
            float f = pos - idx;
            float mod = 0.0f;
            if (idx < (int)p.pitch_env.size() - 1) {
                mod = p.pitch_env[idx] * (1.0f - f) + p.pitch_env[idx + 1] * f;
            } else {
                mod = p.pitch_env.back();
            }
            curFreq *= std::pow(2.0f, (mod - 0.5f) * 2.0f);
        }

        if (p.vibrato_depth > 0.0f) {
            curFreq += std::sin(2.0f * PI_F * p.vibrato_rate * t) * p.vibrato_depth;
        }
        if (p.fm > 0.0f) {
            curFreq += std::sin(2.0f * PI_F * p.fm * t) * curFreq * 0.5f;
        }

        phase += curFreq / SR;
        if (phase > 1.0f) phase -= 1.0f;

        float wave = 0.0f;
        if      (p.waveform == "sine")     wave = std::sin(2.0f * PI_F * phase);
        else if (p.waveform == "square")   wave = (phase < 0.5f) ? 1.0f : -1.0f;
        else if (p.waveform == "sawtooth") wave = 2.0f * phase - 1.0f;
        else                               wave = 1.0f - 4.0f * std::fabs(phase - 0.5f);

        // ОГИБАЮЩАЯ ИЗ ГРАФИЧЕСКОГО РЕДАКТОРА
        float env = 0.0f;
        if (p.vol_env.size() >= 2) {
            float pos = frac * (p.vol_env.size() - 1);
            int idx = (int)pos;
            float f = pos - idx;
            if (idx >= (int)p.vol_env.size() - 1) {
                env = p.vol_env.back();
            } else {
                env = p.vol_env[idx] * (1.0f - f) + p.vol_env[idx + 1] * f;
            }
        } else {
            // Старая ADSR
            if (t < p.attack) env = t / std::max(p.attack, 1e-4f);
            else if (t < p.attack + p.decay) env = 1.0f - (1.0f - p.sustain) * ((t - p.attack) / std::max(p.decay, 1e-4f));
            else if (t < p.duration - p.release) env = p.sustain;
            else env = p.sustain * (1.0f - ((t - (p.duration - p.release)) / std::max(p.release, 1e-4f)));
        }

        float sample = wave * env;

        if (p.noise > 0.0f) {
            float nz = noiseDist(rng);
            sample = sample * (1.0f - p.noise) + nz * p.noise * env;
        }

        if (p.dist_type > 0 && p.dist_drive > 0.0f) {
            float drive = 1.0f + p.dist_drive * 10.0f;
            if (p.dist_type == 1) sample = std::tanh(sample * drive);
            else if (p.dist_type == 2) sample = std::clamp(sample * drive, -0.8f, 0.8f);
            else if (p.dist_type == 3) sample = std::sin(sample * drive * PI_F);
        }

        if (p.bit_depth < 16.0f || p.sample_rate_div > 1.0f) {
            crushCounter++;
            if (crushCounter >= (int)p.sample_rate_div) {
                crushCounter = 0;
                lastCrushed = std::round(sample * steps) / steps;
            }
            sample = lastCrushed;
        }

        if (p.filter_cutoff < 20000.0f) {
            if (p.filter_type == 0) {
                lastOutLP = lastOutLP + alpha * (sample - lastOutLP);
                sample = lastOutLP;
            } else if (p.filter_type == 1) {
                lastOutHP = alpha * (lastOutHP + sample - lastInHP);
                lastInHP = sample;
                sample = lastOutHP + hpRes * p.filter_resonance * 2.0f;
                hpRes = sample * 0.9f + hpRes * 0.1f;
            } else if (p.filter_type == 2) {
                lastOutLP = lastOutLP + alpha * (sample - lastOutLP);
                lastOutHP = alpha * (lastOutHP + sample - lastInHP);
                lastInHP = sample;
                sample = lastOutLP - lastOutHP;
            }
        }

        if (delaySamples > 1) {
            float delayed = delayBuf[delayIdx];
            if (p.ping_pong) {
                pingPongState = -pingPongState;
                delayed *= pingPongState;
            }
            delayBuf[delayIdx] = sample + delayed * p.delay_feedback;
            delayIdx = (delayIdx + 1) % delaySamples;
            sample = sample * (1.0f - p.delay_feedback * 0.5f) + delayed * 0.5f;
        }

        if (p.reverb_mix > 0.0f) {
            float wet = reverb.process(sample, revFeedback);
            sample = sample * (1.0f - p.reverb_mix) + wet * p.reverb_mix;
        }

        if (p.chorus_mix > 0.0f) {
            float lfo = std::sin(2.0f * PI_F * 1.5f * t) * p.chorus_depth * 0.001f * SR;
            int delayTap = (chorusIdx - (int)lfo + chorusMax) % chorusMax;
            float chorusOut = chorusBuf[delayTap];
            
            chorusBuf[chorusIdx] = sample;
            chorusIdx = (chorusIdx + 1) % chorusMax;
            
            sample = sample * (1.0f - p.chorus_mix) + chorusOut * p.chorus_mix;
        }

        if (p.phaser_mix > 0.0f) {
            float fLfo = 0.5f + 0.5f * std::sin(2.0f * PI_F * 0.5f * t); 
            float freq = 200.0f + fLfo * 2000.0f;
            float a = (1.0f - std::tan(PI_F * freq / SR)) / (1.0f + std::tan(PI_F * freq / SR));
            
            float x = sample;
            for (int stage = 0; stage < 4; ++stage) {
                float y = -a * x + phaserBuf[stage];
                phaserBuf[stage] = x + a * y;
                x = y;
            }
            sample = sample * (1.0f - p.phaser_mix) + x * p.phaser_mix;
        }

        if (p.tremolo_rate > 0.0f) {
            float lfo = 0.5f + 0.5f * std::sin(2.0f * PI_F * p.tremolo_rate * t);
            sample *= lfo;
        }

        if (p.ring_mod_freq > 0.0f) {
            float ring = std::sin(2.0f * PI_F * p.ring_mod_freq * t);
            sample *= ring;
        }

        if (p.step_mode) {
            int stepIdx = (int)(t / stepDuration) % 16;
            if (!p.steps[stepIdx]) sample = 0.0f;
        }

        // ПРИМЕНЕНИЕ НАРИСОВАННОЙ ОГИБАЮЩЕЙ
        if (!p.drawn_env.empty()) {
            float pos = frac * (p.drawn_env.size() - 1);
            int idx = (int)pos;
            float f = pos - idx;
            float drawEnv = 1.0f;
            if (idx < (int)p.drawn_env.size() - 1) {
                drawEnv = p.drawn_env[idx] * (1.0f - f) + p.drawn_env[idx+1] * f;
            } else {
                drawEnv = p.drawn_env.back();
            }
            sample *= drawEnv; 
        }

        sample = std::tanh(sample * p.gain * 1.2f) * 0.95f;
        out[i] = sample;
    }

    if (p.seamless_crossfade > 0.0f) {
        int cfSamples = std::min((int)(p.seamless_crossfade * SR), n / 2);
        if (cfSamples > 0) {
            for (int i = 0; i < cfSamples; ++i) {
                float mix = (float)i / cfSamples; 
                out[i] = out[i] * mix + out[n - cfSamples + i] * (1.0f - mix);
            }
        }
    }

    return out;
}