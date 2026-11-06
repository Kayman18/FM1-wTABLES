// Mechanical float version of bundled upstream source for validation.
/*
    SlothCircuit.hpp  -  Don Cross <cosinekitty@gmail.com>  -  2023-09-23

    A software emulation of the "Sloth Chaos" hardware module by Andrew Fitch:
    https://www.nonlinearcircuits.com/modules/p/4hp-sloth-chaos

    The following document describes the circuit schematic, analysis,
    and the mathematical derivation of this emulation algorithm.
    https://github.com/cosinekitty/sloth/blob/main/README.md
*/
#pragma once

#include <algorithm>
#include <cmath>

namespace AnalogFloat
{
    // Op-amp saturation voltages.
    // I measured these from my breadboard build of Sloth on the comparator U1.
    const float QNEG = -10.64f;
    const float QPOS = +11.38f;

    class SlothCircuit
    {
    private:
        const float timeDilation;      // The time dilation factor creates variants that run at different speeds.
        const float w0;                // Initial charge voltage of C3, which affects initial trajectory.

        // Inputs
        float K{};     // the total resistance R3 (fixed) + R9 (variable)
        float U{};     // control voltage fed into the circuit via R8

        // Node voltages
        float x1{};    // voltage at the output of op-amp U3
        float w1{};    // voltage at the top of capacitor C3
        float y1{};    // voltage at the output of op-amp U4
        float z1{};    // voltage at the output of op-amp U2

        // Node voltage increments. We remember these across samples
        // to form an initial guess about how the voltage will change
        // in the next sample. This improves efficiency by converging faster.
        float dx{};
        float dw{};
        float dy{};

        // Capacitor values in farads.
        const float C1 = 2.0e-6f;
        const float C2 = 1.42e-6f;    // schematic says 1uF, but this value acts more "slothy"
        const float C3 = 50.0e-6f;

        // Resistor values in ohms.
        // R3 + R9 = K, so R3 and R9 are not listed here.
        const float R1 = 1.0e+6f;
        const float R2 = 4.7e+6f;
        const float R4 = 100.0e+3f;
        const float R5 = 100.0e+3f;
        const float R6 = 100.0e+3f;
        const float R7 = 100.0e+3f;
        const float R8 = 470.0e+3f;

        // Power supply rail voltages.
        const float VNEG = -12.0f;
        const float VPOS = +12.0f;

        float Q(float z)
        {
            // The comparator U1 output responds immediately to the voltage z.
            // It is an inverting amplifier whose output is saturated.
            return (z < 0.0f) ? QPOS : QNEG;
        }

    protected:
        SlothCircuit(float _timeDilation, float _w0)
            : timeDilation(_timeDilation)
            , w0(_w0)
        {
            initialize();
            setKnobPosition(0.0f);
            setControlVoltage(0.0f);
        }

    public:
        // The iteration safety limit for the convergence solver.
        const int iterationLimit = 5;

        void initialize()
        {
            w1 = w0;
            x1 = y1 = z1 = 0;
            dx = dw = dy = 0;
        }

        void setKnobPosition(float fraction)
        {
            // Clamp the fraction to the range [0, 1].
            float clamped = std::max(0.0f, std::min(1.0f, fraction));

            // The maximum value of the variable resistor is 10K.
            // This is in series with a fixed resistance of 100K.
            K = 100.0e+3f + (clamped * 10.0e+3f);
        }

        void setControlVoltage(float cv)
        {
            // Clamp the control voltage to the circuit's supply rails.
            U = std::max(VNEG, std::min(VPOS, cv));
        }

        float xVoltage() const
        {
            return x1;
        }

        float yVoltage() const
        {
            return y1;
        }

        float zVoltage() const
        {
            return z1;
        }

        int update(float sampleRateHz)      // returns the number of iterations needed for convergence [1..iterationLimit]
        {
            float dt = timeDilation / sampleRateHz;

            // Form an initial guess about the mean voltage during the time interval `dt`.
            // Use linear extrapolation to guess that the voltages will keep changing
            // at the same rate they did in the previous sample.
            float xm = x1 + dx/2;
            float wm = w1 + dw/2;
            float ym = y1 + dy/2;
            float zm = -R4*(ym/R5 + U/R8);
            float Qm = Q(zm);

            // Iterate until convergence.
            const float tolerance = 1.0e-12f;        // one picovolt
            const float toleranceSquared = tolerance * tolerance;

            float ex, ew, ey;
            for (int iter = 1; true; ++iter)
            {
                // Remember the previous delta voltages, so we can tell whether we have converged next time.
                ex = dx;
                ew = dw;
                ey = dy;

                // Update the finite changes of the voltage variables after the time interval.
                dx = -dt/C1 * (zm/R1 + Qm/R2 + wm/K);
                dw = dt/C3*(xm/R6-(1/R6 + 1/K + 1/R7)*wm);
                dy = (-dt/(R7*C2)) * wm;

                float x2 = x1 + dx;
                float w2 = w1 + dw;
                float y2 = y1 + dy;

                // Assume z changes instantaneously because there is no capacitor the U2 feedback loop.
                float z2 = -R4*(y2/R5 + U/R8);

                // Has the solver converged?
                // Calculate how much the deltas have changed since last time.
                float ddx = dx - ex;
                float ddw = dw - ew;
                float ddy = dy - ey;
                float variance = ddx*ddx + ddw*ddw + ddy*ddy;
                if (variance < toleranceSquared || iter >= iterationLimit)
                {
                    // The solution has converged, or we have hit the iteration safety limit.
                    // Update the circuit state voltages and return.
                    x1 = x2;
                    w1 = w2;
                    y1 = y2;
                    z1 = z2;
                    return iter;
                }

                // We approximate the mean value over the time interval as the average
                // of the starting value with the estimated next value.
                xm = x1 + dx/2;
                wm = w1 + dw/2;
                zm = (z1 + z2)/2;

                // Usually Q remains constant, but it toggles when z changes polarity.
                if (z1 * z2 >= 0)
                {
                    Qm = Q(zm);
                }
                else
                {
                    // alpha = the fraction into the time step at which z(t) = 0.f
                    float alpha = z1 / (z1 - z2);
                    Qm = alpha*Q(z1) + (1-alpha)*Q(z2);
                }
            }
        }
    };


    class TorporSlothCircuit : public SlothCircuit
    {
    public:
        TorporSlothCircuit()
            : SlothCircuit(1.0f, 0.0f)
            {}
    };


    class ApathySlothCircuit : public SlothCircuit
    {
    public:
        ApathySlothCircuit()
            : SlothCircuit(0.27391343022607395f, +0.017f)
            {}
    };


    class InertiaSlothCircuit : public SlothCircuit
    {
    public:
        InertiaSlothCircuit()
            : SlothCircuit(0.009697118406631193f, -0.023f)
            {}
    };


    class TripleSlothCircuit
    {
    public:
        TorporSlothCircuit  torpor;
        ApathySlothCircuit  apathy;
        InertiaSlothCircuit inertia;

        void initialize()
        {
            torpor.initialize();
            apathy.initialize();
            inertia.initialize();
        }

        void update(float sampleRateHz)
        {
            torpor.update(sampleRateHz);
            apathy.update(sampleRateHz);
            inertia.update(sampleRateHz);
        }

        float zsum() const
        {
            float t = torpor.zVoltage();
            float a = apathy.zVoltage();
            float i = inertia.zVoltage();
            return 0.7f*(t - (a+i));
        }
    };
}
