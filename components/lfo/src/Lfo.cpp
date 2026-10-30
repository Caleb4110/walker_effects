#include <cmath>
#include <walker/lfo/Lfo.h>

namespace walker::lfo
{
void Lfo::Init(float _sampleRate)
{
    sampleRate = _sampleRate;
    phase = 0.0f;
}

void Lfo::SetFrequency(float hz) { phaseInc = hz / sampleRate; }

// returns -1..1, call once per sample
float Lfo::Next()
{
    float twoPi = 6.28318530718f;
    const float out = std::sin(twoPi * phase);
    phase += phaseInc;
    if (phase >= 1.0f) phase -= 1.0f;
    return out;
}

float Lfo::NextTri()
{
    const float out = 4.0f * std::fabs(phase - 0.5f) - 1.0f;
    phase += phaseInc;
    if (phase >= 1.0f) phase -= 1.0f;
    return out;
}
}
