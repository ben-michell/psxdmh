// psxdmh/src/options.h
// Command line options for psxdmh.
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

#ifndef PSXDMH_SRC_OPTIONS_H
#define PSXDMH_SRC_OPTIONS_H


#include "command_line.h"
#include "reverb.h"


namespace psxdmh
{


// Options controlling the behaviour of psxdmh.
class options : public command_line
{
public:

    // Construction.
    options();

private:

    // Custom callbacks to handle special options.
    void handle_volume(std::string value);
    void handle_reverb_preset(std::string value);
    void handle_reverb_volume(std::string value);
    void handle_stereo_expansion(std::string value);

public:

    // - - - - - - - - - - - - Volume adustment options - - - - - - - - - - - -

    // Volume scaling (amplitude).
    mono_t volume;

    // Apply level normalization.
    bool normalize;

    // - - - - - - - - - - - - - - Playback options - - - - - - - - - - - - - -

    // Reverb configuration. Volume is in amplitude form.
    reverb_preset reverb_preset;
    mono_t reverb_volume;

    // Number of times to play a repeating song, track or patch. A value of 0
    // means repeat indefinitely. Other values play exactly that many times.
    uint32_t play_count;

    // - - - - - - - - - - - Silence adjustment options - - - - - - - - - - -

    // Amount of leading and trailing silence to enforce on songs and tracks.
    // Negative values indicate not set.
    double lead_in;
    double lead_out;

    // Maximum silent gap allowed in songs and tracks. A negative value
    // indicates not set.
    double maximum_gap;

    // - - - - - - - - - Audio repair and adjustment options - - - - - - - - -

    // Stereo width adjustment.
    mono_t stereo_width;

    // Automatic fixing of patches with audio faults.
    bool repair_patches;

    // Ignore the maximum playback frequency limit of a real PSX.
    bool unlimited_frequency;

    // - - - - - - - - - - - - - - Output options - - - - - - - - - - - - - -

    // Output sample rate.
    uint32_t sample_rate;

    // High-pass and low-pass frequencies for filtering the generated audio. A
    // value of 0 means that filter is not required.
    uint32_t high_pass;
    uint32_t low_pass;

    // Resampling configuration.
    uint32_t sinc_window;

    // - - - - - - - - - - - - - Miscellaneous options - - - - - - - - - - - - -

    // Display version and license information.
    bool version;

    // Display usage information.
    bool help;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_OPTIONS_H
