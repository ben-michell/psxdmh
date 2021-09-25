// psxdmh/src/statistics.h
// Audio statistics collection module.
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

#ifndef PSXDMH_SRC_STATISTICS_H
#define PSXDMH_SRC_STATISTICS_H


#include "module.h"


namespace psxdmh
{


// Statistics collection modes.
enum class statistics_mode
{
    progress,    // Measure only progress of audio generation.
    detailed     // All statistics.
};


// Audio statistics collection module.
template <typename S> class statistics : public module<S>
{
public:

    // Callback used to report progress. The amount of audio generated is given
    // by seconds. The rate of extraction (song time relative to wall time) is
    // given by rate. This value will be 0 until sufficient data has been
    // generated to give a proper estimate. The user_data value is supplied by
    // the caller to the constructor of the statistics module.
    typedef void (*callback)(uint32_t seconds, double rate, std::string operation);

    // Construction.
    statistics(module<S> *source, statistics_mode mode, uint32_t rate, callback progress_callback, std::string callback_operation) :
        module<S>(source),
        m_mode(mode),
        m_rate(rate),
        m_callback(progress_callback), m_callback_operation(callback_operation),
        m_start_time(0),
        m_last_rate_time(0), m_extraction_rate(0),
        m_samples(0),
        m_samples_until_next_second(rate),
        m_maximum(0),
        m_rms_total(0)
    {
        assert(rate > 0);
    }

    // Test whether the module is still generating output.
    virtual bool is_running() const { return this->source()->is_running(); }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // Track the number of samples extracted. Start the timer on the first
        // extraction.
        if (m_samples++ == 0)
        {
            m_start_time = clock();
        }
        bool live = this->source()->next(s);

        if (m_mode == statistics_mode::detailed)
        {
            // Record the maximum sample magnitude.
            m_maximum = std::max(m_maximum, magnitude(s));

            // Accumulate the RMS total. Despite this involving the addition of
            // a large number of small values, there is sufficient precision in
            // the double type to give a far more accurate result than required.
            m_rms_total += pow(magnitude(s), 2);
        }

        // Update the progress callback once per second of extracted audio.
        if (--m_samples_until_next_second == 0)
        {
            // Update the extraction rate every half wall second.
            m_samples_until_next_second = m_rate;
            uint32_t song_seconds = uint32_t(m_samples / m_rate);
            clock_t elapsed = clock() - m_start_time;
            uint32_t elapsed_half_seconds = uint32_t(2 * elapsed / CLOCKS_PER_SEC);
            if (elapsed_half_seconds != m_last_rate_time)
            {
                m_extraction_rate = clamp(double(song_seconds) / (double(elapsed) / CLOCKS_PER_SEC), 0.0, 1000000.0);
                m_last_rate_time = elapsed_half_seconds;
            }

            if (m_callback != nullptr)
            {
                m_callback(song_seconds, m_extraction_rate, m_callback_operation);
            }
        }
        return live;
    }

    // Last calculated extraction rate. This will be 0 until sufficient data has
    // been generated to give a proper estimate.
    double extraction_rate() const { return m_extraction_rate; }

    // Maximum sample magnitude encountered during processsing. Only valid when
    // the module is being run in detailed mode.
    mono_t maximum_amplitude() const { assert(m_mode == statistics_mode::detailed); return m_maximum; }
    double maximum_db() const { assert(m_mode == statistics_mode::detailed); return amplitude_to_decibels(m_maximum); }

    // RMS of the audio in dB. Only valid when the module is being run in
    // detailed mode.
    double rms_db() { assert(m_mode == statistics_mode::detailed); return m_samples > 0 ? amplitude_to_decibels(sqrt(m_rms_total / m_samples)) : 0.0; }

private:

    // Mode of operation.
    statistics_mode m_mode;

    // Rate of the audio in samples per second.
    uint32_t m_rate;

    // Progress callback.
    callback m_callback;
    std::string m_callback_operation;

    // Time when the first sample was processed.
    clock_t m_start_time;

    // Elapsed seconds when the extraction rate was last calculated.
    uint32_t m_last_rate_time;

    // Last calculated extraction rate. Updated once per wall second.
    double m_extraction_rate;

    // Number of samples processed.
    uint64_t m_samples;

    // Number of samples to be processed until the next second of extracted
    // audio is complete.
    uint32_t m_samples_until_next_second;

    // Maximum sample magnitude encountered during processsing.
    mono_t m_maximum;

    // Sum of the squares of the magnitude of all samples processed.
    double m_rms_total;
};


// Types for mono and stereo statistics modules.
typedef statistics<mono_t> statistics_mono;
typedef statistics<stereo_t> statistics_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_STATISTICS_H
