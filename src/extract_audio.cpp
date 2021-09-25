// psxdmh/src/extract_audio.cpp
// Extract songs, tracks, and patches.
// Copyright (c) 2021 Ben Michell <https://www.muryan.com/>. All rights reserved.

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

#include "adpcm.h"
#include "channel.h"
#include "extract_audio.h"
#include "lcd_file.h"
#include "normalizer.h"
#include "options.h"
#include "reverb.h"
#include "silencer.h"
#include "song_player.h"
#include "statistics.h"
#include "track_player.h"
#include "utility.h"
#include "volume.h"
#include "wav_file.h"
#include "wmd_file.h"


namespace psxdmh
{


// Ctrl-C can't be caught on Windows as it causes the creation of a new thread
// to specifically handle the SIGINT interrupt.
#if !defined(PSXDMH_TARGET_WINDOWS)
    #define PSXDMH_CATCH_CTRL_C
#endif // Target.


// Forwards.
static void extract_music(module_stereo *source, uint16_t song_index, std::string wav_file_name, const options &opts);
static module_stereo *construct_graph(module_stereo *module, uint16_t song_index, std::string wav_file_name, const options &opts, statistics_stereo *&statistics, normalizer_stereo *&normalizer);
static uint32_t write_wav_file(module_stereo *module, std::string wav_file_name, const options &opts);
static void display_music_statistics(const options &opts, uint32_t ticks, song_player *song_module, statistics_stereo *statistics, normalizer_stereo *normalizer);
static std::string default_song_name(uint16_t song_index);
static void default_reverb(uint16_t song_index, reverb_preset &preset, mono_t &volume);
static void status_callback(uint32_t seconds, double rate, std::string operation);
#ifdef PSXDMH_CATCH_CTRL_C
static void signal_handler(int);
#endif // PSXDMH_CATCH_CTRL_C


// Global setjmp buffer.
#ifdef PSXDMH_CATCH_CTRL_C
static jmp_buf g_env;
#endif // PSXDMH_CATCH_CTRL_C


//
// Extract a range of songs.
//

void
extract_songs(const std::vector<uint16_t> &song_indexes, const wmd_file &wmd, const lcd_file &lcd, std::string output_name, const options &opts)
{
    // Extract the songs.
    for (auto iter = song_indexes.cbegin(); iter != song_indexes.cend(); ++iter)
    {
        assert(*iter < wmd.songs());
        if (iter != song_indexes.begin())
        {
            message::writef(verbosity::normal, "\n");
        }
        std::string wav_name = !output_name.empty() ? output_name : default_song_name(*iter);
        message::writef(verbosity::normal, "Extracting song %u (%s)\n", *iter, wav_name.c_str());
        module_stereo *module = new song_player(*iter, wmd, lcd, opts);
        extract_music(module, *iter, wav_name, opts);
    }
}


//
// Extract one track from a song.
//

void
extract_track(uint16_t song_index, uint16_t track_index, const wmd_file &wmd, const lcd_file &lcd, std::string wav_file_name, const options &opts)
{
    // Validate the song and track indexes.
    if (song_index >= wmd.songs())
    {
        throw std::string("Invalid song index.");
    }
    if (track_index >= wmd.song(song_index).tracks.size())
    {
        throw std::string("Invalid track index.");
    }

    // Create the track player and extract the music.
    module_stereo *module = new track_player(song_index, track_index, wmd, lcd, opts);
    extract_music(module, song_index, wav_file_name, opts);
}


//
// Extract a range of patches from an LCD file.
//

void
extract_patch(const std::vector<uint16_t> &patch_ids, const lcd_file &lcd, std::string output_name, const options &opts)
{
    // Extract the patches.
    for (auto iter = patch_ids.cbegin(); iter != patch_ids.cend(); ++iter)
    {
        const patch *patch = lcd.patch_by_id(*iter);
        if (patch == nullptr)
        {
            // A missing patch is fatal when extracting a single patch, but just
            // a warning when extracting a range. This makes it easier to
            // extract a range of patches where some are missing.
            std::string error_msg = std::string("Invalid patch ID ") + int_to_string(*iter) + ".";
            if (patch_ids.size() == 1)
            {
                throw error_msg;
            }
            message::writef(verbosity::normal, "\nWarning: %s\n", error_msg.c_str());
            continue;
        }

        // Decode the patch and store it as a WAV file.
        if (iter != patch_ids.begin())
        {
            message::writef(verbosity::normal, "\n");
        }
        std::string wav_name = !output_name.empty() ? output_name : std::string("Patch ") + int_to_string(*iter) + ".wav";
        message::writef(verbosity::normal, "Extracting patch %u (%s)\n", *iter, wav_name.c_str());
        std::unique_ptr<module_mono> module(new adpcm(patch->adpcm, opts.play_count));
        wav_file_mono wav_file_writer;
        uint32_t length = wav_file_writer.write(module.get(), wav_name, opts.sample_rate);
        message::writef(verbosity::normal, "Extracted %u samples (%.3lf seconds).\n", length, length / double(opts.sample_rate));
    }
}


//
// Handle the common part of song and track extraction. This function takes
// ownership of the module.
//

static void
extract_music(module_stereo *source, uint16_t song_index, std::string wav_file_name, const options &opts)
{
    // Remember if module was a song_player.
    assert(source != nullptr);
    song_player *song_module = dynamic_cast<song_player *>(source);

    // Construct the graph of audio modules.
    statistics_stereo *statistics;
    normalizer_stereo *normalizer;
    std::unique_ptr<module_stereo> module(construct_graph(source, song_index, wav_file_name, opts, statistics, normalizer));

    // Extract the music and display a summary of what was written.
    uint32_t ticks = write_wav_file(module.get(), wav_file_name, opts);
    display_music_statistics(opts, ticks, song_module, statistics, normalizer);
}


//
// Construct the graph of audio modules to extract music.
//

static module_stereo *
construct_graph(module_stereo *module, uint16_t song_index, std::string wav_file_name, const options &opts, statistics_stereo *&statistics, normalizer_stereo *&normalizer)
{
    // Decide whether to show progress messages. This is only done when the
    // verbosity is high enough, and when the output is going to a terminal.
    assert(module != nullptr);
    bool show_progress = message::verbosity() >= verbosity::normal && is_interactive(stdout);

    // Add maximum gap processing. This needs to be done before reverb to
    // prevent the reverb effect from prolonging the gaps.
    if (opts.maximum_gap >= 0.0)
    {
        int32_t gap = std::max(int32_t(opts.maximum_gap * opts.sample_rate), 1);
        module = new silencer_stereo(module, -1, -1, gap);
    }

    // Add reverb.
    reverb_preset preset = opts.reverb_preset;
    mono_t reverb_volume = opts.reverb_volume;
    if (preset == rp_auto)
    {
        default_reverb(song_index, preset, reverb_volume);
        if (reverb_volume > 0.0)
        {
            message::writef(verbosity::verbose, "Reverb defaulted to %s at %.1lf dB.\n", reverb_to_string(preset).c_str(), amplitude_to_decibels(reverb_volume));
        }
    }
    if (preset != rp_off)
    {
        module = new reverb(module, opts.sample_rate, preset, reverb_volume, opts.sinc_window);
    }

    // Add lead-in and lead-out processing. The lead-out needs to be done after
    // reverb to avoid cutting off echoes. Lead-in doesn't matter either way.
    if (opts.lead_in >= 0.0 || opts.lead_out >= 0.0)
    {
        // Make sure that if lead-in or lead-out are used, then they're at least
        // one sample in length to ensure the song starts or ends on silence.
        int32_t lead_in = opts.lead_in >= 0.0 ? std::max(int32_t(opts.lead_in * opts.sample_rate), 1) : -1;
        int32_t lead_out = opts.lead_out >= 0.0 ? std::max(int32_t(opts.lead_out * opts.sample_rate), 1) : -1;
        module = new silencer_stereo(module, lead_in, lead_out, -1);
    }

    // Add filtering.
    if (opts.high_pass != 0)
    {
        module = new filter_stereo(module, filter_type::high_pass, double(opts.high_pass) / opts.sample_rate);
    }
    if (opts.low_pass != 0)
    {
        module = new filter_stereo(module, filter_type::low_pass, double(opts.low_pass) / opts.sample_rate);
    }

    // Apply normalization, and use a statistics module to report progress of
    // the extraction upstream of the normalizer.
    normalizer = nullptr;
    if (opts.normalize)
    {
        if (message::verbosity() >= verbosity::normal)
        {
            module = new statistics_stereo(module, statistics_mode::progress, opts.sample_rate, status_callback, "Extracted");
        }
        normalizer = new normalizer_stereo(module, wav_file_name + ".tmp");
        module = normalizer;
    }

    // Add volume adjustment.
    if (opts.volume != 1.0)
    {
        module = new volume_stereo(module, opts.volume);
    }

    // Display progress and collect statistics if required.
    statistics = nullptr;
    if (message::verbosity() >= verbosity::normal)
    {
        statistics_mode mode = message::verbosity() >= verbosity::verbose ? statistics_mode::detailed : statistics_mode::progress;
        statistics_stereo::callback callback = show_progress ? status_callback : nullptr;
        std::string operation = opts.normalize ? "Normalized" : "Extracted";
        statistics = new statistics_stereo(module, mode, opts.sample_rate, callback, operation);
        module = statistics;
    }
    channel::reset_maximum_channels();
    return module;
}


//
// Write the output of an audio module to a WAV file.
//

static uint32_t
write_wav_file(module_stereo *module, std::string wav_file_name, const options &opts)
{
    assert(module != nullptr);
    uint32_t ticks;
    wav_file_stereo wav_file_writer;
    try
    {
#ifdef PSXDMH_CATCH_CTRL_C
        // Convert SIGINT into an exception to provoke the clean up code.
        if (setjmp(g_env) != 0)
        {
            // Throw an exception to terminate.
            signal(SIGINT, SIG_DFL);
            throw std::string("Aborted.");
        }
        signal(SIGINT, signal_handler);
#endif // PSXDMH_CATCH_CTRL_C

        // Extract the music.
        ticks = wav_file_writer.write(module, wav_file_name, opts.sample_rate);

#ifdef PSXDMH_CATCH_CTRL_C
        // Remove the signal handler.
        signal(SIGINT, SIG_DFL);
#endif // PSXDMH_CATCH_CTRL_C
    }
    catch (...)
    {
        // If an error occurs remove the WAV file.
        if (wav_file_writer.is_file_open())
        {
            message::writef(verbosity::verbose, "Aborting: removing '%s'.\n", wav_file_name.c_str());
            wav_file_writer.abort();
        }
        throw;
    }
    return ticks;
}


//
// Display statistics about music extraction.
//

static void
display_music_statistics(const options &opts, uint32_t ticks, song_player *song_module, statistics_stereo *statistics, normalizer_stereo *normalizer)
{
    // Display a summary of what was written.
    std::string time = ticks_to_time(ticks, opts.sample_rate);
    double extraction_rate = statistics != nullptr ? statistics->extraction_rate() : 0.0;
    if (extraction_rate > 0)
    {
        message::writef(verbosity::normal, "%s: %s (%.1lfx)    \n", "Extracted", time.c_str(), extraction_rate);
    }
    else
    {
        message::writef(verbosity::normal, "%s: %s                \n", "Extracted", time.c_str());
    }
    if (normalizer != nullptr)
    {
        message::writef(verbosity::verbose, "  Normalization: %.1lf dB\n", normalizer->adjustment_db());
    }
    if (message::verbosity() >= verbosity::verbose)
    {
        message::writef(verbosity::verbose, "  Maximum Channels: %d\n", channel::maximum_channels());
    }
    if (message::verbosity() >= verbosity::verbose && statistics != nullptr)
    {
        message::writef(verbosity::verbose, "  Maximum Level: %.1lf dB / %.1lf%%\n", statistics->maximum_db(), statistics->maximum_amplitude() * 100.0);
        message::writef(verbosity::verbose, "  RMS: %.1lf dB\n", statistics->rms_db());
    }
    if (song_module != nullptr && song_module->failed_to_repeat())
    {
        assert(opts.play_count > 1);
        message::writef(verbosity::normal, "Warning: song does not contain a repeat point; play-count ignored.\n");
    }
}


//
// Create a default name for a song. For music this is the name of the level
// where the song is first used. All other songs are sound effects.
//

static std::string
default_song_name(uint16_t song_index)
{
    static const std::string default_song_names[120] =
    {
        "SFX00 - Silence",
        "SFX01 - Shotgun Load",
        "SFX02 - Punch",
        "SFX03 - Item Respawn",
        "SFX04 - Fireball Launch (Unused)",
        "SFX05 - Barrel Explosion",
        "SFX06 - Lost Soul Death",
        "SFX07 - Pistol Fire",
        "SFX08 - Shotgun Fire",
        "SFX09 - Plasma Fire",
        "SFX10 - BFG9000 Fire",
        "SFX11 - Chainsaw Raise",
        "SFX12 - Chainsaw Idle",
        "SFX13 - Chainsaw Full Power",
        "SFX14 - Chainsaw Hit",
        "SFX15 - Rocket Launcher Fire",
        "SFX16 - BFG9000 Explosion",
        "SFX17 - Platform Start",
        "SFX18 - Platform Stop",
        "SFX19 - Door Open",
        "SFX20 - Door Close",
        "SFX21 - Stone Move",
        "SFX22 - Switch Normal",
        "SFX23 - Switch Exit",
        "SFX24 - Item Pick Up",
        "SFX25 - Weapon Pick Up",
        "SFX26 - Player Oof",
        "SFX27 - Teleport",
        "SFX28 - Player Grunt",
        "SFX29 - Super Shotgun Fire",
        "SFX30 - Super Shotgun Open",
        "SFX31 - Super Shotgun Load",
        "SFX32 - Super Shotgun Close",
        "SFX33 - Player Pain",
        "SFX34 - Player Death",
        "SFX35 - Slop",
        "SFX36 - Zombieman Alert 1",
        "SFX37 - Zombieman Alert 2",
        "SFX38 - Zombieman Alert 3",
        "SFX39 - Zombieman Death 1",
        "SFX40 - Zombieman Death 2",
        "SFX41 - Zombieman Death 3",
        "SFX42 - Zombieman Active",
        "SFX43 - Zombieman Pain",
        "SFX44 - Demon Pain",
        "SFX45 - Demon Active",
        "SFX46 - Imp Attack",
        "SFX47 - Imp Alert 1",
        "SFX48 - Imp Alert 2",
        "SFX49 - Imp Death 1",
        "SFX50 - Imp Death 2",
        "SFX51 - Imp Active",
        "SFX52 - Demon Alert",
        "SFX53 - Demon Attack",
        "SFX54 - Demon Death",
        "SFX55 - Baron Of Hell Alert",
        "SFX56 - Baron Of Hell Death",
        "SFX57 - Cacodemon Alert",
        "SFX58 - Cacodemon Death",
        "SFX59 - Lost Soul Attack",
        "SFX60 - Lost Soul Death",
        "SFX61 - Hell Knight Alert",
        "SFX62 - Hell Knight Death",
        "SFX63 - Pain Elemental Alert",
        "SFX64 - Pain Elemental Pain",
        "SFX65 - Pain Elemental Death",
        "SFX66 - Arachnotron Alert",
        "SFX67 - Arachnotron Death",
        "SFX68 - Arachnotron Active",
        "SFX69 - Arachnotron Walk",
        "SFX70 - Mancubus Attack",
        "SFX71 - Mancubus Alert",
        "SFX72 - Mancubus Pain",
        "SFX73 - Mancubus Death",
        "SFX74 - Fireball Launch",
        "SFX75 - Revenant Alert",
        "SFX76 - Revenant Death",
        "SFX77 - Revenant Active",
        "SFX78 - Revenant Attack",
        "SFX79 - Revenant Swing",
        "SFX80 - Revenant Punch",
        "SFX81 - Cyberdemon Alert",
        "SFX82 - Cyberdemon Death",
        "SFX83 - Cyberdemon Walk",
        "SFX84 - Spider Mastermind Walk",
        "SFX85 - Spider Mastermind Alert",
        "SFX86 - Spider Mastermind Death",
        "SFX87 - Blaze Door Open",
        "SFX88 - Blaze Door Close",
        "SFX89 - Get Power-Up",
        "D01 - Hangar",             // 90
        "D02 - Plant",              // 91
        "D03 - Toxin Refinery",     // 92
        "D04 - Command Control",    // 93
        "D05 - Phobos Lab",         // 94
        "D06 - Central Processing", // 95
        "D07 - Computer Station",   // 96
        "D08 - Phobos Anomaly",     // 97
        "D10 - Containment Area",   // 98
        "D12 - Deimos Lab",         // 99
        "D09 - Deimos Anomaly",     // 100
        "D16 - Hell Gate",          // 101
        "D21 - Mt. Erebus",         // 102
        "D22 - Limbo",              // 103
        "D11 - Refinery",           // 104
        "D17 - Hell Keep",          // 105
        "D18 - Pandemonium",        // 106
        "D20 - Unholy Cathedral",   // 107
        "D13 - Command Center",     // 108
        "D24 - Hell Beneath",       // 109
        "F05 - Catwalk",            // 110
        "F09 - Nessus",             // 111
        "F01 - Attack",             // 112
        "F03 - Canyon",             // 113
        "F07 - Geryon",             // 114
        "F10 - Paradox",            // 115
        "F06 - Fistula",            // 116
        "F08 - Minos",              // 117
        "F02 - Virgil",             // 118
        "F04 - Combine"             // 119
    };

    if (song_index < numberof(default_song_names))
    {
        return std::string(default_song_names[song_index]) + ".wav";
    }
    assert(!"Unhandled song name.");
    return std::string("S") + int_to_string(song_index) + ".wav";
}


//
// Get the default reverb settings for a song.
//

static void
default_reverb(uint16_t song_index, reverb_preset &preset, mono_t &volume)
{
    // Reverb configuration for each level. Since these values are set by each
    // level of the game, they do not appear in any of the audio data files.
    // Also it appears that the reverb settings change as the player moves
    // between different areas in a level (indoors and outdoors for example).
    struct reverb_config
    {
        reverb_preset preset;
        uint16_t depth;
    };
    static const reverb_config reverb[30] =
    {
        { rp_space_echo,    0x0fff },   // 90
        { rp_space_echo,    0x0fff },   // 91
        { rp_studio_medium, 0x27ff },   // 92
        { rp_hall,          0x17ff },   // 93
        { rp_studio_small,  0x23ff },   // 94
        { rp_hall,          0x1fff },   // 95
        { rp_studio_large,  0x26ff },   // 96
        { rp_studio_medium, 0x2dff },   // 97
        { rp_studio_large,  0x2fff },   // 98
        { rp_space_echo,    0x0fff },   // 99
        { rp_hall,          0x1fff },   // 100
        { rp_hall,          0x1fff },   // 101
        { rp_space_echo,    0x0fff },   // 102
        { rp_hall,          0x1fff },   // 103
        { rp_studio_medium, 0x27ff },   // 104
        { rp_space_echo,    0x0fff },   // 105
        { rp_hall,          0x1fff },   // 106
        { rp_space_echo,    0x0fff },   // 107
        { rp_hall,          0x1fff },   // 108
        { rp_studio_large,  0x2fff },   // 109
        { rp_space_echo,    0x1fff },   // 110
        { rp_space_echo,    0x1fff },   // 111
        { rp_hall,          0x1fff },   // 112
        { rp_space_echo,    0x1fff },   // 113
        { rp_space_echo,    0x0fff },   // 114
        { rp_space_echo,    0x0fff },   // 115
        { rp_hall,          0x1fff },   // 116
        { rp_hall,          0x1fff },   // 117
        { rp_studio_large,  0x26ff },   // 118
        { rp_space_echo,    0x0fff }    // 119
    };

    const size_t first_song = 90;
    if (song_index >= first_song && song_index < (first_song + numberof(reverb)))
    {
        preset = reverb[song_index - first_song].preset;
        volume = mono_t(reverb[song_index - first_song].depth) / 0x7fff;
    }
    else
    {
        preset = rp_off;
        volume = 0.0;
    }
}


//
// Callback to display the time of an operation.
//

static void
status_callback(uint32_t seconds, double rate, std::string operation)
{
    if (rate == 0.0)
    {
        message::writef(verbosity::normal, "  %s: %2u:%02u                \r", operation.c_str(), seconds / 60, seconds % 60);
    }
    else
    {
        message::writef(verbosity::normal, "  %s: %2u:%02u (%.1lfx)    \r", operation.c_str(), seconds / 60, seconds % 60, rate);
    }
}


#ifdef PSXDMH_CATCH_CTRL_C

//
// Signal handler to longjmp back to the caller.
//

static void
signal_handler(int)
{
    longjmp(g_env, 1);
}

#endif // PSXDMH_CATCH_CTRL_C


}; //namespace psxdmh
