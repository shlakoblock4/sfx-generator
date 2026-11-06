#pragma once
#include <vector>
#include "audio_params.h"

constexpr int   SR = 44100;
constexpr float PI_F = 3.14159265358979323846f;

// Главная функция синтеза
std::vector<float> synthesize(const Params& p);