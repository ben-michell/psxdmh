// psxdmh/src/envelope.h
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

#ifndef PSXDMH_SRC_ENVELOPE_H
#define PSXDMH_SRC_ENVELOPE_H


#include "module.h"


namespace psxdmh
{


// PSX SPU ADSR envelope emulation.
class envelope : public module_mono
{
public:

    // Construction. The envelope starts the attack phase immediately. The two
    // parameters correspond to the two SPU ADSR registers.
    envelope(uint16_t spu_ads, uint16_t spu_sr);

    // Test if the envelope is currently running. Once started, the envelope
    // will run until the release phase drops the envelope volume to 0.
    virtual bool is_running() const { return m_phase != ep_stopped; }

    // Get the next sample. The returned value ranges from 0.0 to 1.0 and
    // represents the envelope volume.
    virtual bool next(mono_t &mono);

    // Start the release phase. Unlike the other phases, release is explicitly
    // triggered.
    void release();

    // Dump details about the envelope.
    void dump(std::string indent);

    // Sample rate used by the envelope module.
    static uint32_t sample_rate() { return 44100; }

private:

    // Envelope phase.
    enum phase
    {
        ep_attack,
        ep_decay,
        ep_sustain,
        ep_release,

        // Number of active phases: stopped doesn't count.
        ep_number_of_phases,

        ep_stopped = ep_number_of_phases
    };

    // Envelope change method.
    enum class method
    {
        linear,
        exponential
    };

    // Envelope change direction.
    enum class direction
    {
        increase,
        decrease
    };

    // Configuration describing how to run each phase.
    struct config
    {
        method method;
        direction direction;
        int32_t shift;
        int32_t step;
        int32_t target;
    };

    // Calculate the next wait and step cycle.
    void calculate_cycle();

    // Convert a method or a direction to a string.
    static std::string method_str(method m) { return m == method::linear ? "linear" : "exponential"; }
    static std::string direction_str(direction d) { return d == direction::increase ? "increase" : "decrease"; }

    // Configuration for each phase.
    config m_config[ep_number_of_phases];

    // Current phase.
    phase m_phase;

    // Current envelope volume: 0x0000 - 0x7fff.
    int32_t m_volume;

    // Current cycle within the current phase: the number of times to repeat it,
    // the number of ticks to wait each step, and the step to apply to the
    // volume after each wait.
    uint32_t m_cycle_repeats;
    uint32_t m_cycle_wait;
    uint32_t m_cycle_current_wait;
    int32_t m_cycle_step;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_ENVELOPE_H
