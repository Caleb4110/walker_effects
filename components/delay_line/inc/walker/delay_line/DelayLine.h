// DelayLine.h
#pragma once

#include <vector>

namespace walker::delay_line
{
class DelayLine {
private:
    std::vector<float>  delayBuffer;
    int                 mask            = 0;    // delayBuffer.size() - 1 (power of two)
    int                 writeIdx        = 0;
    int                 delayInSamples  = 0;
    float               sampleRate      = 0.0f;
public:
    void    Init(float _sampleRate, float _maxDelaySeconds);
    void    SetDelay(float _delayInSamples);
    float   Process(float in);
    void    ProcessBlock(float* data, int numSamples);
};
}
