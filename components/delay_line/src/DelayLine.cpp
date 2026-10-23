#include <walker/delay_line/DelayLine.h>
#include <algorithm>

namespace walker::delay_line
{
void DelayLine::Init(float _sampleRate, float maxDelaySeconds)
{
    sampleRate = _sampleRate;

    // round up to a power of two so wrapping is a cheap bitmask
    int needed = static_cast<int>(maxDelaySeconds * sampleRate) + 1;
    int size = 1;
    while (size < needed) size <<= 1;

    delayBuffer.assign(size, 0.0f);   // allocates + zeroes (replaces memset)
    mask = size - 1;
    writeIdx = 0;
    delayInSamples = 0;
}

void DelayLine::SetDelay(float _delayInSamples)
{
    // TODO: parameter smoothing
    delayInSamples = std::clamp(_delayInSamples, 0.0f, static_cast<float>(mask - 1));
}

float DelayLine::Process(float in)
{
    delayBuffer[writeIdx] = in;

    const int   whole = static_cast<int>(delayInSamples);
    const float frac  = delayInSamples - static_cast<float>(whole);

    const int idx0 = (writeIdx - whole)     & mask;
    const int idx1 = (writeIdx - whole - 1) & mask;

    writeIdx = (writeIdx + 1) & mask;

    return delayBuffer[idx0] + frac * (delayBuffer[idx1] - delayBuffer[idx0]);
}

void DelayLine::ProcessBlock(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        data[i] = Process(data[i]);
}
}
