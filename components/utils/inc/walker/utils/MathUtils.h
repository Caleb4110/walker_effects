#pragma once

namespace walker::utils
{
    /**
     * @brief Performs linear interpolation between (x0, y0) and (x1, y1) at
     *        position x. Returns y0 directly when x0 == x1 to avoid a
     *        divide-by-zero.
     *
     * @param x   The interpolation position.
     * @param x0  Left-hand x coordinate.
     * @param y0  Left-hand y value.
     * @param x1  Right-hand x coordinate.
     * @param y1  Right-hand y value.
     * @return    Interpolated value at x.
     */
    inline float lerp_f(float x, int x0, float y0, int x1, float y1)
    {
        float out;

        // if both points are equal, just return the already found value
        if (x1 == x0)
        {
            out = y0;
        }
        else
        {
            out = (y0 * (x1 - x) + y1 * (x - x0)) / (x1 -x0);
        }

        return out;
    }

    /**
     * @brief 4-point, 3rd-order Hermite (Catmull-Rom) interpolation.
     *
     * Interpolates between y0 and y1 using ym1 (the point before y0) and
     * y2 (the point after y1) to shape the curve.
     *
     * @param t    Fractional position between y0 and y1, in [0, 1).
     * @param ym1  Sample before y0.
     * @param y0   Sample at t = 0.
     * @param y1   Sample at t = 1.
     * @param y2   Sample after y1.
     * @return     Interpolated value at t.
     */
    inline float hermite_f(float t, float ym1, float y0, float y1, float y2)
    {
        const float c0 = y0;
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * t + c2) * t + c1) * t + c0;
    }
}
