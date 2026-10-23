#include <walker/dsp/Smoother.h>

namespace walker::dsp
{
    float smooth (float current, float target, float coeff)
    {
        return current + coeff * (target - current);
    }
}
