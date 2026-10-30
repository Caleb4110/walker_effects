#include <walker/delay_line/DelayLine.h>
#include <walker/utils/MathUtils.h>
#include <algorithm>

namespace walker::delay_line
{
void DelayLine::Init(float _sampleRate, float maxDelaySeconds)
{
    sampleRate = _sampleRate;

    int needed = static_cast<int>(maxDelaySeconds * sampleRate) + 4; // a little headroom for the 4-point read
    int size = 1;
    while (size < needed) size <<= 1;

    delayBuffer.assign(static_cast<size_t>(size), 0.0f);
    mask = size - 1;
    writeIdx = 0;
    delayInSamples = 1.0f;
}

void DelayLine::SetDelay(float _delayInSamples)
{
    // Hermite reads one sample newer and two older than the base tap,
    // so the delay must stay within [1, mask - 2]
    delayInSamples = std::clamp(_delayInSamples, 1.0f, static_cast<float>(mask - 2));
}

float DelayLine::Process(float in)
{
    float   out;
    int     whole;
    float   t;

    delayBuffer[(unsigned int)writeIdx] = in;

    whole = static_cast<int>(delayInSamples);
    t     = delayInSamples - static_cast<float>(whole);

    // Lambda stored at variable 'at'
    // [&] = capture list (allows the lambda to use the a copy of the 
    //                     surrounding function's variables.
    //                     '[]' would not allow to use surrounding
    //                     variables)
    auto at = [&](int relativeOffset) -> float
    {
        return delayBuffer[static_cast<size_t>((writeIdx - whole - relativeOffset) & mask)];
    };

    out = walker::utils::hermite_f(t, at(-1), at(0), at(1), at(2));

    writeIdx = (writeIdx + 1) & mask;

    return out;
}

void DelayLine::ProcessBlock(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        data[i] = Process(data[i]);
}
}
