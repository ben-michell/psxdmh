// psxdmh/src/filter.h
// Butterworth IIR filtering.
// Copyright (c) 2016,2021 Ben Michell <https://www.muryan.com/>. All rights reserved.

// This file is part of psxdmh.
//
// psxdmh is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// psxdmh is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// psxdmh.  If not, see <http://www.gnu.org/licenses/>.

#ifndef PSXDMH_SRC_FILTER_H
#define PSXDMH_SRC_FILTER_H


#include "module.h"


namespace psxdmh
{


// Filter types.
enum class filter_type
{
    low_pass,
    high_pass
};


// Butterworth IIR filtering (second order).
//
// This filter reduces the amplitude of the source by -3.01 dB at the cut off
// frequency. This equates to a reduction in amplitude to 0.7071. The response
// of the low-pass filter around the cut off is as follows (invert the octave
// offset for the high-pass form):
//
// Octave     Reduction (dB : amplitude)
// ---------  --------------------------
// cut_off-1   -0.25 dB : 0.9716
// cut_off     -3.01 dB : 0.7071
// cut_off+1   -12 dB   : 0.2512
// cut_off+2   -24 dB   : 0.0631
// cut_off+3   -36 dB   : 0.0158
template <typename S> class filter : public module<S>
{
public:

    // Construction. The cut_off represents the frequency as a fraction of the
    // sample rate where the filter reduces the amplitude by -3 dB. The cut_off
    // must be in the range [0.0, 0.5).
    filter(module<S> *source, filter_type type, double cut_off) :
        module<S>(source),
        m_type(type)
    {
        // Initialize the filter coefficients and clear the filter.
        assert(source != nullptr);
        assert(type == filter_type::low_pass || type == filter_type::high_pass);
        assert(cut_off >= 0.0 && cut_off < 0.5);
        adjust(cut_off);
        clear();
    }

    // Test whether the module is still generating output.
    virtual bool is_running() const
    {
        // Run while the filter is non-silent or the source is running.
        return !is_silent(m_x1) || !is_silent(m_x2) || !is_silent(m_y1) || !is_silent(m_y2)
            || this->source()->is_running();
    }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // Calculate the new output value.
        S source_sample;
        bool source_live = this->source()->next(source_sample);
        s = flush_denorm(m_a0 * source_sample + m_a1 * m_x1 + m_a2 * m_x2 - m_b1 * m_y1 - m_b2 * m_y2);

        // Shift the stored previous values along by one.
        m_x2 = m_x1;
        m_x1 = source_sample;
        m_y2 = m_y1;
        m_y1 = s;
        return source_live || !is_silent(m_x1) || !is_silent(m_x2) || !is_silent(m_y1) || !is_silent(m_y2);
    }

    // Set a new cut off without clearing the filter.
    void adjust(double cut_off)
    {
        // Calculate the raw coefficients for a second order Butterworth.
        assert(cut_off >= 0.0 && cut_off < 0.5);
        double w0 = 2.0 * M_PI * cut_off;
        double cos_w0 = cos(w0);
        double alpha = sin(w0) / M_SQRT2;
        double b0 = 1.0 + alpha;
        double b1 = -2.0 * cos_w0;
        double b2 = 1.0 - alpha;
        double a0, a1;
        if (m_type == filter_type::low_pass)
        {
            a0 = 0.5 * (1.0 - cos_w0);
            a1 = 1.0 - cos_w0;
        }
        else
        {
            assert(m_type == filter_type::high_pass);
            a0 = 0.5 * (1.0 + cos_w0);
            a1 = -1.0 - cos_w0;
        }

        // Normalize and store the filter coefficients. We want the filter to
        // have 0 dB gain in the passband, so normalize b0 to 1.0. Then
        // a0 + a1 + a2 + b1 + b2 will equal 1.0.
        assert(b0 < -0.000001 || b0 > 0.000001);
        m_a0 = mono_t(a0 / b0);
        m_a1 = mono_t(a1 / b0);
        m_a2 = mono_t(a0 / b0);
        m_b1 = mono_t(b1 / b0);
        m_b2 = mono_t(b2 / b0);
    }

private:

    // Clear the filter's previous input and output values.
    void clear()
    {
        m_x1 = 0.0;
        m_x2 = 0.0;
        m_y1 = 0.0;
        m_y2 = 0.0;
    }

    // Filter type: high pass or low pass.
    filter_type m_type;

    // Coefficients of x.
    mono_t m_a0;
    mono_t m_a1;
    mono_t m_a2;

    // Coefficients of y.
    mono_t m_b1;
    mono_t m_b2;

    // Previous samples.
    S m_x1;
    S m_x2;

    // Previous filter output.
    S m_y1;
    S m_y2;
};


// Types for mono and stereo filters.
typedef filter<mono_t> filter_mono;
typedef filter<stereo_t> filter_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_FILTER_H
