// psxdmh/src/resampler.h
// Resampling of audio to arbitrary frequencies.
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

#ifndef PSXDMH_SRC_RESAMPLER_H
#define PSXDMH_SRC_RESAMPLER_H


#include "filter.h"
#include "module.h"


namespace psxdmh
{


// Base class for resamplers.
template <typename S> class resampler : public module<S>
{
public:

    // Construction. The rate_in and rate_out values control the resampling.
    // Their actual values are irrelevant; all that matters is their ratio. The
    // two rates are interpreted such that the pitch of the source audio at
    // rate_in retains the same pitch when output from this module at rate_out.
    resampler(module<S> *source, uint32_t rate_in, uint32_t rate_out) :
        module<S>(source),
        m_rate_in(rate_in), m_rate_out(rate_out)
    {
        assert(rate_in > 0);
        assert(rate_out > 0);
    }

    // Input sample rate.
    uint32_t rate_in() const { return m_rate_in; }
    virtual void rate_in(uint32_t new_rate) { assert(new_rate > 0); m_rate_in = new_rate; }

    // Output sample rate.
    uint32_t rate_out() const { return m_rate_out; }
    virtual void rate_out(uint32_t new_rate) { assert(new_rate > 0); m_rate_out = new_rate; }

private:

    // Input sample rate.
    uint32_t m_rate_in;

    // Output sample rate.
    uint32_t m_rate_out;
};


// Types for mono and stereo resamplers.
typedef resampler<mono_t> resampler_mono;
typedef resampler<stereo_t> resampler_stereo;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Linear resampling. This should not be used on actual audio data as it will
// produce poor quality sound. It is, however, fine for resampling the envelope
// since it is quite linear in nature.
template <typename S> class resampler_linear : public resampler<S>
{
public:

    // Construction.
    resampler_linear(module<S> *source, uint32_t rate_in, uint32_t rate_out) :
        resampler<S>(source, rate_in, rate_out),
        m_fractional_position(0),
        m_last_live_sample(1)
    {
        // Prepare the resampler.
        assert(source != nullptr);
        source->next(m_sample_buffer[0]);
        source->next(m_sample_buffer[1]);
    }

    // Test whether the resampler is still generating output. The resampler runs
    // until the last real sample from the source has moved out of the buffer.
    virtual bool is_running() const { return m_last_live_sample >= 0; }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // Return silence when not running.
        if (m_last_live_sample < 0)
        {
            s = 0.0;
            return false;
        }

        // Get the waveform at the current position. First handle the case of
        // being exactly on a source sample.
        const uint32_t step = this->rate_out();
        assert(m_fractional_position < step);
        if (m_fractional_position == 0)
        {
            s = m_sample_buffer[0];
        }
        // The position is between two source samples, so interpolate.
        else
        {
            assert(m_fractional_position > 0 && m_fractional_position < step);
            mono_t pos = m_fractional_position / mono_t(step);
            s = (1 - pos) * m_sample_buffer[0] + pos * m_sample_buffer[1];
        }

        // Advance through the data and replenish the buffer if required.
        m_fractional_position += this->rate_in();
        while (m_fractional_position >= step && m_last_live_sample >= 0)
        {
            // Shuffle the contents of the buffer down, and add the new sample.
            assert(!this->source()->is_running() || m_last_live_sample == 1);
            m_fractional_position -= step;
            m_sample_buffer[0] = m_sample_buffer[1];
            if (!this->source()->next(m_sample_buffer[1]))
            {
                // When the source has finished keep track of the last real
                // sample.
                assert(m_last_live_sample >= 0);
                m_last_live_sample--;
            }
        }
        return true;
    }

private:

    // Current fractional position between samples. There are rate_out()
    // fractional steps between samples.
    uint32_t m_fractional_position;

    // Buffered samples. The buffer is always filled.
    S m_sample_buffer[2];

    // Remember the buffer index of the last real source sample.
    int m_last_live_sample;
};


// Types for mono and stereo linear resamplers.
typedef resampler_linear<mono_t> resampler_linear_mono;
typedef resampler_linear<stereo_t> resampler_linear_stereo;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Helper class to cache pre-computed sinc values. These tables are expensive to
// prepare, so caching speeds up the calculations considerably.
class sinc_table : public uncopyable
{
public:

    // Obtain a table of sinc values.
    static const sinc_table &obtain(uint32_t window, uint32_t rate_out);

    // Sinc configuration.
    uint32_t window() const { return m_window; }
    uint32_t rate_out() const { return m_rate_out; }

    // Get the table of values. Use the index_for_offset method to locate the
    // starting index within this table, where there will be window * 2 values.
    const std::vector<float> &table() const { return m_table; }

    // Starting table index for an offset. The offset must be in the range
    // [0, m_rate_out).
    size_t index_for_offset(int32_t offset) const { assert(offset >= 0 && offset < (int32_t) m_rate_out); return size_t(offset * m_window * 2); }

private:

    // Construction. This is private as these objects are managed by the class
    // itself.
    sinc_table(uint32_t window, uint32_t rate_out);

private:

    // Private list of cached tables. Once a table is added to the cache it is
    // never released. Usually there should only be at most two tables in the
    // cache: one to convert samples to the output rate, and another to convert
    // to the reverb rate.
    static sinc_table *m_cache;

    // Next table in the cache.
    sinc_table *m_next;

    // Sinc configuration.
    uint32_t m_window;
    uint32_t m_rate_out;

    // Table of pre-computed sinc values.
    std::vector<float> m_table;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Resample with a Lanczos windowed sinc filter. Note that when audio is being
// resampled to a lower rate the source should ideally be pre-filtered to remove
// frequencies that would exceed the Nyquist limit in the resampled output,
// which is to say, any frequencies in excess of half the output rate.
template <typename S> class resampler_sinc : public resampler<S>
{
public:

    // Construction. The window size must be at least 1. A value of 7 gives
    // high-quality results, while a value of 3 gives generally satisfactory
    // results though some artifacts will be audible. The speed of this
    // resampler is proportional to the window size.
    resampler_sinc(module<S> *source, uint32_t window, uint32_t rate_in, uint32_t rate_out) :
        resampler<S>(source, rate_in, rate_out),
        m_window((int32_t) window),
        m_circular_buffer(window * 2, 0), m_buffer_head(0),
        m_offset(0),
        m_live_samples(window * 2),
        m_table(sinc_table::obtain(window, rate_out))
    {
        // Prepare the resampler. The buffer repeats the first sample up to
        // where the position is 0, then it starts pulling in new samples. This
        // gives the resampler no delay on start-up.
        assert(source != nullptr);
        assert(window >= 1);
        source->next(m_circular_buffer[0]);
        int32_t pos = -int32_t(rate_out) * (m_window - 1);
        for (size_t index = 1; index < m_circular_buffer.size(); ++index)
        {
            if (pos <= 0)
            {
                m_circular_buffer[index] = m_circular_buffer[0];
            }
            else
            {
                source->next(m_circular_buffer[index]);
            }
            pos += this->rate_out();
        }
    }

    // Test whether the resampler is still generating output. The resampler runs
    // until there are no more live samples in the window.
    virtual bool is_running() const { return m_live_samples > 0; }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // Return silence if the resampler is not running.
        s = 0.0;
        if (m_live_samples <= 0)
        {
            return false;
        }

        // Calculate the interpolated value at this position.
        assert(m_offset >= 0);
        assert(m_offset < (int32_t) this->rate_out());
        const std::vector<float> &table = m_table.table();
        size_t buffer_index = m_buffer_head;
        size_t table_index = m_table.index_for_offset(m_offset);
        size_t table_end = table_index + m_window * 2;
        for (; table_index < table_end; ++table_index)
        {
            assert(table_index >= 0);
            assert(table_index < table.size());
            assert(buffer_index < m_circular_buffer.size());
            s += m_circular_buffer[buffer_index] * table[table_index];
            if (++buffer_index >= m_circular_buffer.size())
            {
                assert(buffer_index == m_circular_buffer.size());
                buffer_index = 0;
            }
        }
        s = flush_denorm(s);

        // Advance the filter.
        m_offset += this->rate_in();
        const int32_t limit = int32_t(this->rate_out());
        while (m_offset >= limit)
        {
            // Obtain the next sample from the source if it is still running,
            // otherwise repeat the last sample.
            m_offset -= limit;
            assert(m_buffer_head < m_circular_buffer.size());
            if (!this->source()->next(m_circular_buffer[m_buffer_head]))
            {
                const size_t previous = m_buffer_head > 0 ? m_buffer_head - 1 : m_circular_buffer.size() - 1;
                m_circular_buffer[m_buffer_head] = m_circular_buffer[previous];
                m_live_samples--;
            }

            if (++m_buffer_head >= m_circular_buffer.size())
            {
                m_buffer_head = 0;
            }
        }
        return true;
    }

private:

    // Window size. Samples in the range (-m_window, m_window) are included in
    // the filter.
    int32_t m_window;

    // Buffered samples. The size of the buffer is twice the window size. The
    // buffer is always filled. The first sample in the buffer is always less
    // than the window size to the left of the interpolation position and is
    // located at the offset m_buffer_head. As this is a circular buffer, the
    // samples wrap.
    std::vector<S> m_circular_buffer;
    size_t m_buffer_head;

    // Offset of the first buffered sample relative to the interpolation
    // position and (m_window - 1) windows. This value is measured in fractions
    // of a sample, where there are rate_out() steps per sample. The offset is
    // stored instead of the actual position since that's all we need to locate
    // the sinc table entries. The actual position can be calculated from this
    // offset by: -(m_window - 1) * rate_out() - m_offset
    int32_t m_offset;

    // Number of live samples in the buffer. The resampler stops when there are
    // no more real samples left in the buffer.
    int m_live_samples;

    // Table with pre-computed sinc values.
    const sinc_table &m_table;
};


// Types for mono and stereo sinc resamplers.
typedef resampler_sinc<mono_t> resampler_sinc_mono;
typedef resampler_sinc<stereo_t> resampler_sinc_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_RESAMPLER_H
