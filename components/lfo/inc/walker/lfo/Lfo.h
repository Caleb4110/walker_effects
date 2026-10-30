#pragma once

namespace walker::lfo
{
class Lfo {
private:
    float phase     = 0.0f;   // 0..1
    float phaseInc  = 0.0f;
    float sampleRate = 0.0f;
public:
    void Init(float _sampleRate);
    void SetFrequency(float hz);
    float Next();
    float NextTri();
};
}
