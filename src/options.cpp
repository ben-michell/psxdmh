// psxdmh/src/options.cpp
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

#include "global.h"

#include "options.h"
#include "utility.h"


namespace psxdmh
{


//
// Construction.
//

options::options() :
    volume(1.0),
    normalize(false),
    reverb_preset(rp_auto), reverb_volume(0.5),
    play_count(1L),
    lead_in(-1.0), lead_out(-1.0),
    maximum_gap(-1.0),
    stereo_width(0.0),
    repair_patches(false),
    unlimited_frequency(false),
    sample_rate(0),
    high_pass(30L), low_pass(15000L),
    sinc_window(7L),
    version(false),
    help(false)
{
    // Volume adustment options.
    define_callback_option("volume", 'v', new custom_string_callback<options>(*this, &options::handle_volume), "dB",
        "Set the amplification of the output in dB (default 0).  "
        "This can be combined with the -n option in which case this volume adjustment occurs after the normalization.");
    define_bool_option("normalize", 'n', normalize,
        "Normalize the level of the audio to use the full range.  "
        "This option writes the audio to a temporary file, and requires approximately twice the space of the completed WAV file.");

    // Playback options.
    define_callback_option("reverb-preset", 'r', new custom_string_callback<options>(*this, &options::handle_reverb_preset), "preset",
        "Set which reverb effect to use (default auto).  "
        "Valid values are studio-small, studio-medium, studio-large, half-echo, space-echo, hall, room, off, and auto.  "
        "Selecting off disables the effect.  "
        "Selecting auto will set the reverb preset and volume to the values used by the game level where the song first appears.");
    define_callback_option("reverb-volume", 'R', new custom_string_callback<options>(*this, &options::handle_reverb_volume), "dB",
        "Set the volume of the reverb effect in dB (default -6).  "
        "This option has no effect if the reverb preset is set to off or auto.");
    define_uint_option("play-count", 'p', play_count, 1U, UINT32_MAX, "count",
        "Set the number of times a repeating song, track, or patch is played (default 1).");

    // Silence adjustment options.
    define_double_option("intro", 'i', lead_in, 0.0, 60.0, "time",
        "Enforce a silent period of exactly the given time at the start of a song (default off).  "
        "This will add or remove silence as required to give the specified amount.");
    define_double_option("outro", 'o', lead_out, 0.0, 60.0, "time",
        "Enforce a silent period of exactly the given time at the end of a song (default off).  "
        "This will add or remove silence as required to give the specified amount.");
    define_double_option("maximum-gap", 'g', maximum_gap, 1.0, 60.0, "time",
        "Limit the length of silent periods within songs or tracks to the given time (default off).  "
        "Some songs, such as song 95, contain excessively long silences.  "
        "This option can be used to reduce these gaps to a more reasonable length.");

    // Audio repair and adjustment options.
    define_callback_option("stereo-expansion", 'x', new custom_string_callback<options>(*this, &options::handle_stereo_expansion), "width",
        "Adjust the width of the stereo effect (default 0.0).  "
        "A value of -1.0 reduces the audio to near mono, 0.0 leaves it unchanged, and 1.0 pushes any uncentred sound to the far left or far right.");
    define_bool_option("repair-patches", 'P', repair_patches,
        "Repair patches with major audio faults such as clicks, pops, and excessive noise where possible.  "
        "Songs 94, 97, 98, 102, 106, 113, and 114 all use patches that are repaired by this option.");
    define_bool_option("unlimited", 'u', unlimited_frequency,
        "Real PlayStation hardware has a limit to how much it can raise the pitch of sounds.  "
        "Several songs, such as song 95, contain notes that try to exceed this limit.  "
        "Setting this option removes the limit.");

    // Output options.
    define_uint_option("sample-rate", 's', sample_rate, 8000U, 192000U, "rate",
        "Set the output sample rate (default 44100 for songs and tracks, 11025 for patches).");
    define_uint_option("high-pass", 'h', high_pass, 0U, 192000U, "frequency",
        "Attenuate frequencies lower than the given frequency in the output (default 30).  "
        "A value of 0 disables the filter.");
    define_uint_option("low-pass", 'l', low_pass, 0U, 192000U, "frequency",
        "Attenuate frequencies higher than the given frequency in the output (default 15000).  "
        "A value of 0 disables the filter.");
    define_uint_option("sinc-window", 'w', sinc_window, 1U, UINT32_MAX, "size",
        "Set the size of the sinc resampling window (default 7).  "
        "This controls the audio quality when the pitch of a sound is changed.  "
        "A value of 7 gives high-quality results.  "
        "Higher values give slightly better results at the expense of more processing time.  "
        "A value of 3 gives satisfactory results for most songs and is faster, though some songs will contain audible artifacts.");

    // Miscellaneous options.
    define_verbosity_option("quiet", 'Q', verbosity::quiet, "Display errors only.");
    define_verbosity_option("verbose", 'V', verbosity::verbose, "Display extended information.");
    define_bool_option("version", 0, version, "Display version and license information.");
    define_bool_option("help", 0, help, "Display help text.");
}


//
// Custom callback to handle volume.
//

void
options::handle_volume(std::string value)
{
    double db = string_to_double(value.c_str(), -100.0, 100.0, "volume");
    volume = (mono_t) decibels_to_amplitude(db);
}


//
// Custom callback to handle reverb preset.
//

void
options::handle_reverb_preset(std::string value)
{
    // Valid values for the reverb preset.
    static const std::pair<std::string, psxdmh::reverb_preset> reverb_preset_str[] =
    {
        std::make_pair(reverb_to_string(rp_studio_small), rp_studio_small),
        std::make_pair(reverb_to_string(rp_studio_medium), rp_studio_medium),
        std::make_pair(reverb_to_string(rp_studio_large), rp_studio_large),
        std::make_pair(reverb_to_string(rp_half_echo), rp_half_echo),
        std::make_pair(reverb_to_string(rp_space_echo), rp_space_echo),
        std::make_pair(reverb_to_string(rp_hall), rp_hall),
        std::make_pair(reverb_to_string(rp_room), rp_room),
        std::make_pair(reverb_to_string(rp_off), rp_off),
        std::make_pair("auto", rp_auto)
    };

    auto end = reverb_preset_str + numberof(reverb_preset_str);
    auto predicate = [&value](const std::pair<std::string, psxdmh::reverb_preset> &r) { return value == r.first; };
    auto preset = std::find_if(reverb_preset_str, end, predicate);
    if (preset == end)
    {
        throw std::string("Unknown reverb preset '") + value + "'.";
    }
    reverb_preset = preset->second;
}


//
// Custom callback to handle reverb volume.
//

void
options::handle_reverb_volume(std::string value)
{
    double db = string_to_double(value.c_str(), -100.0, 100.0, "reverb-volume");
    reverb_volume = (mono_t) decibels_to_amplitude(db);
}


//
// Custom callback to handle stereo expansion.
//

void
options::handle_stereo_expansion(std::string value)
{
    stereo_width = (mono_t) string_to_double(value.c_str(), -1.0, 1.0, "stereo-expansion");
}


}; //namespace psxdmh
