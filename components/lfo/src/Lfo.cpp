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
    const float out = std::sin(6.28318530718f * phase);
    phase += phaseInc;
    if (phase >= 1.0f) phase -= 1.0f;
    return out;
}
}
