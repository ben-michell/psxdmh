// psxdmh/src/silencer.h
// Silence adjustement: lead in, lead out, and gaps.
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

#ifndef PSXDMH_SRC_SILENCER_H
#define PSXDMH_SRC_SILENCER_H


#include "module.h"


namespace psxdmh
{


// Silence adjustement. This module can adjust the lead-in, lead-out, and gaps
// within the audio.
template <typename S> class silencer : public module<S>
{
public:

    // Construction. The lead_in and lead_out are the number of silent samples
    // to enforce at the start and end of audio. The gap is the maximum length
    // allowed for silent periods between non-silent audio. Negative values
    // deactivate the respective settings. If gap is set then it must be at
    // least 1, otherwise it will interfere with waveforms that cross the 0
    // level.
    silencer(module<S> *source, int32_t lead_in, int32_t lead_out, int32_t gap) :
        module<S>(source),
        m_lead_in(lead_in),
        m_lead_out(lead_out),
        m_gap(gap),
        m_state(state::lead_in),
        m_buffered_silence(0),
        m_have_unsilent_sample(false)
    {
        assert(source != nullptr);
        assert(gap != 0);
    }

    // Test whether the module is still generating output.
    virtual bool is_running() const
    {
        // Process more audio to top up the buffers if required. This module is
        // still running only if there is something buffered after this.
        if (m_buffered_silence == 0 && !m_have_unsilent_sample && m_state != state::finished)
        {
            process_audio();
        }
        return m_buffered_silence > 0 || m_have_unsilent_sample;
    }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // Process more audio if the buffers are empty.
        if (m_buffered_silence == 0 && !m_have_unsilent_sample && m_state != state::finished)
        {
            process_audio();
        }

        // Buffered silence is always output first.
        if (m_buffered_silence > 0)
        {
            m_buffered_silence--;
            s = 0.0;
            return true;
        }
        // Output any non-silent sample.
        else if (m_have_unsilent_sample)
        {
            m_have_unsilent_sample = false;
            s = m_unsilent_sample;
            return true;
        }

        // All audio and silences have been processed. This module is no longer
        // running.
        assert(m_state == state::finished);
        s = 0.0;
        return false;
    }

private:

    // Audio processing states.
    enum class state
    {
        lead_in,
        gaps,
        lead_out,
        finished
    };

    // Process audio from the source. This must only be called when there is no
    // buffered audio (silent or unsilent).
    void process_audio() const
    {
        // Attempt to buffer the next unsilent sample.
        assert(m_buffered_silence == 0 && !m_have_unsilent_sample);
        while (!m_have_unsilent_sample)
        {
            if (!this->source()->next(m_unsilent_sample))
            {
                assert(!this->source()->is_running());
                break;
            }

            if (is_silent(m_unsilent_sample))
            {
                m_buffered_silence++;
            }
            else
            {
                m_have_unsilent_sample = true;
            }
        }

        // Handle gaps. Any time an unsilent sample is found any buffered
        // silence is limited to the maximum gap. When no unsilent sample is
        // found lead out begins.
        if (m_state == state::gaps)
        {
            if (m_have_unsilent_sample)
            {
                if (m_gap >= 0 && (int32_t) m_buffered_silence > m_gap)
                {
                    m_buffered_silence = m_gap;
                }
            }
            else
            {
                // Immediately fall through to the lead out code below.
                m_state = state::lead_out;
            }
        }

        // Handle lead out. Buffer the requested amount of silence and finish.
        if (m_state == state::lead_out)
        {
            assert(!this->source()->is_running());
            if (m_lead_out >= 0)
            {
                m_buffered_silence = m_lead_out;
            }
            m_state = state::finished;
        }

        // Handle lead in. This happens only the first time through when no
        // unsilent sample has yet been seen.
        if (m_state == state::lead_in)
        {
            assert(m_have_unsilent_sample || !this->source()->is_running());
            if (m_lead_in >= 0)
            {
                m_buffered_silence = m_lead_in;
            }
            m_state = m_have_unsilent_sample ? state::gaps : state::lead_out;
        }
    }

    // Number of silent samples to enforce at the start and end of audio.
    // Silence will be added or removed as required to achieve the desired
    // amount. A negative value will deactivate the setting.
    int32_t m_lead_in;
    int32_t m_lead_out;

    // Maximum length allowed for silent periods between non-silent audio. A
    // negative value deactivates the setting.
    int32_t m_gap;

    // Current state of the audio processing.
    mutable state m_state;

    // Number of silent samples awaiting output.
    mutable uint32_t m_buffered_silence;

    // Buffered non-silent sample. This always follows any buffered silence.
    mutable bool m_have_unsilent_sample;
    mutable S m_unsilent_sample;
};


// Types for mono and stereo silencers.
typedef silencer<mono_t> silencer_mono;
typedef silencer<stereo_t> silencer_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_SILENCER_H
