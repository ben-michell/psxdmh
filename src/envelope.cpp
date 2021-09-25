// psxdmh/src/envelope.cpp
// PSX SPU ADSR envelope emulation.
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

#include "global.h"

#include "envelope.h"


namespace psxdmh
{


//
// Construction.
//

envelope::envelope(uint16_t spu_ads, uint16_t spu_sr) :
    m_phase(ep_attack),
    m_volume(0),
    m_cycle_repeats(1), m_cycle_wait(1), m_cycle_current_wait(1), m_cycle_step(0)
{
    // Configure each phase.
    m_config[ep_attack].method = (spu_ads & 0x8000) == 0 ? method::linear : method::exponential;
    m_config[ep_attack].direction = direction::increase;
    m_config[ep_attack].shift = (spu_ads >> 10U) & 0x1f;
    m_config[ep_attack].step = 7 - ((spu_ads >> 8U) & 0x03);
    m_config[ep_attack].target = 0x7fff;

    m_config[ep_decay].method = method::exponential;
    m_config[ep_decay].direction = direction::decrease;
    m_config[ep_decay].shift = (spu_ads >> 4U) & 0x0f;
    m_config[ep_decay].step = -8;
    m_config[ep_decay].target = int32_t((spu_ads & 0x0f) + 1) * 0x800;

    // Sustain uses a dummy target level that will never be reached as the
    // transition from sustain to release is always triggered explicitly.
    m_config[ep_sustain].method = (spu_sr & 0x8000) == 0 ? method::linear : method::exponential;
    m_config[ep_sustain].direction = (spu_sr & 0x4000) == 0 ? direction::increase : direction::decrease;
    m_config[ep_sustain].shift = (spu_sr >> 8U) & 0x1f;
    m_config[ep_sustain].step = m_config[ep_sustain].direction == direction::increase ? 7 - ((spu_sr >> 6U) & 0x03) : -8 + int32_t((spu_sr >> 6U) & 0x03);
    m_config[ep_sustain].target = m_config[ep_sustain].direction == direction::increase ? 0x8000 : -1;

    m_config[ep_release].method = (spu_sr & 0x20) == 0 ? method::linear : method::exponential;
    m_config[ep_release].direction = direction::decrease;
    m_config[ep_release].shift = spu_sr & 0x1f;
    m_config[ep_release].step = -8;
    m_config[ep_release].target = 0;
}


//
// Get the next sample.
//

bool
envelope::next(mono_t &mono)
{
    // Use the current volume for the level returned this tick.
    assert(m_phase != ep_stopped || m_volume == 0);
    mono = mono_t(m_volume) / 0x7fff;
    bool running = m_phase != ep_stopped;
    if (running)
    {
        // Advance the current cycle. When the wait reaches 0 apply the step.
        assert(m_cycle_current_wait > 0);
        if (--m_cycle_current_wait == 0)
        {
            m_volume = clamp(m_volume + m_cycle_step, 0, 0x7fff);

            // Repeat the same wait and step if required.
            assert(m_cycle_repeats > 0);
            if (--m_cycle_repeats > 0)
            {
                m_cycle_current_wait = m_cycle_wait;
            }
            // Otherwise begin a new cycle.
            else
            {
                // Advance to the next phase when the target level is reached.
                bool reached = m_config[m_phase].direction == direction::increase ? m_volume >= m_config[m_phase].target : m_volume <= m_config[m_phase].target;
                if (reached)
                {
                    m_phase = phase(m_phase + 1);
                }
                if (m_phase != ep_stopped)
                {
                    calculate_cycle();
                }
            }
        }
    }
    return running;
}


//
// Start the release phase.
//

void
envelope::release()
{
    // If the envelope is still runnning then start the release phase.
    if (m_phase != ep_stopped)
    {
        m_phase = ep_release;
        calculate_cycle();
    }
}


//
// Calculate the next wait and step cycle.
//

void
envelope::calculate_cycle()
{
    // Calculate the next step in the emulation. The ADSR generator operates by
    // the simple method of calculating a series of wait times and steps, where
    // the step is applied to the volume level after the number of wait ticks
    // has elapsed. The phase is advanced automatically (except release) when
    // the volume reaches the target level for the current phase.
    assert(m_phase != ep_stopped);
    const config &config = m_config[m_phase];
    m_cycle_wait = 1U << std::max(config.shift - 11, 0);
    m_cycle_step = (int32_t) (uint32_t(config.step) << std::max(11 - config.shift, 0));
    assert(m_cycle_step != 0);
    if (config.method == method::exponential)
    {
        // Exponential increase isn't really exponential: it just changes to a
        // slower rate above 0x6000.
        if (config.direction == direction::increase && m_volume > 0x6000)
        {
            m_cycle_wait *= 4;
        }
        else if (config.direction == direction::decrease)
        {
            m_cycle_step = (m_cycle_step * m_volume) >> 15;
            assert(m_cycle_step != 0 || m_volume == 0);
        }
    }

    // The wait and step values can be rather more coarse than they need to be.
    // Try to break them down to smaller pieces to give a smoother envelope. The
    // method used here will handle decomposition by any power of 2 (wait time
    // is always a power of 2).
    m_cycle_repeats = 1;
    while ((m_cycle_wait & 0x01) == 0 && m_cycle_step != 0 && (m_cycle_step & 0x01) == 0)
    {
        m_cycle_repeats <<= 1;
        m_cycle_wait >>= 1;
        assert(m_cycle_wait > 0);
        m_cycle_step >>= 1;
        assert(m_cycle_step != 0);
    }
    m_cycle_current_wait = m_cycle_wait;
}


//
// Dump details about an ADSR envelope.
//

void
envelope::dump(std::string indent)
{
    printf("%sA: %s, shift %d, step %d\n", indent.c_str(), method_str(m_config[ep_attack].method).c_str(), m_config[ep_attack].shift, m_config[ep_attack].step);
    printf("%sD: shift %d\n", indent.c_str(), m_config[ep_decay].shift);
    printf("%sS: %s %s, level $%04x, shift %d, step %d\n", indent.c_str(), method_str(m_config[ep_sustain].method).c_str(), direction_str(m_config[ep_sustain].direction).c_str(), m_config[ep_decay].target, m_config[ep_sustain].shift, m_config[ep_sustain].step);
    printf("%sR: %s, shift %d\n", indent.c_str(), method_str(m_config[ep_release].method).c_str(), m_config[ep_release].shift);
}


}; //namespace psxdmh
