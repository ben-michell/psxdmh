// psxdmh/src/psxdmh.cpp
// PSX Doom music extraction hack.
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

#include "command_line.h"
#include "enum_dir.h"
#include "extract_audio.h"
#include "lcd_file.h"
#include "message.h"
#include "options.h"
#include "utility.h"
#include "version.h"
#include "wmd_file.h"


// Default to the psxdmh namespace.
using namespace psxdmh;


namespace psxdmh
{


// Forwards.
static void handle_extract_songs(const std::vector<std::string> &args, options &opts);
static void handle_extract_track(const std::vector<std::string> &args, options &opts);
static void handle_extract_patch(const std::vector<std::string> &args, options &opts);
static void handle_dump_lcd(const std::vector<std::string> &args, options &opts);
static void handle_dump_wmd(const std::vector<std::string> &args, options &opts);
static void handle_dump_song(const std::vector<std::string> &args, options &opts);
static void handle_pack_data(const std::vector<std::string> &args, options &opts);
static void show_version();
static void show_help();
static void load_lcd(std::string file_name, lcd_file &lcd, const options &opts);
static void load_wmd(std::string file_name, wmd_file &wmd, const options &opts);
static void load_music_dir(std::string music_dir, wmd_file &wmd, lcd_file &lcd, const options &opts, bool root = true);
static void validate_filters(const options &opts);
static void check_arg_count(const std::vector<std::string> &args, size_t min_args, size_t max_args, std::string what);


// Actions.
static const std::string g_action_song = "song";
static const std::string g_action_track = "track";
static const std::string g_action_patch = "patch";
static const std::string g_action_dump_lcd = "dump-lcd";
static const std::string g_action_dump_wmd = "dump-wmd";
static const std::string g_action_dump_song = "dump-song";
static const std::string g_action_pack_data = "pack-data";


// Default sample rates.
static const uint32_t g_sample_rate_song = 44100;
static const uint32_t g_sample_rate_patch = 11025;


}; //namespace psxdmh


//
// Entry point.
//

int
main(int argc, char *argv[])
{
    // Show help if the command line is empty.
    if (argc <= 1)
    {
        show_help();
        return 0;
    }

    try
    {
        // Parse the command line.
        options opts;
        std::vector<std::string> args;
        opts.parse(argc, argv, args);

        // Display help.
        if (opts.help)
        {
            show_help();
        }
        // Display version.
        if (opts.version)
        {
            show_version();
        }

        // Handle the requested action.
        std::string action = !args.empty() ? args[0] : "";
        if (action == g_action_song)
        {
            handle_extract_songs(args, opts);
        }
        else if (action == g_action_track)
        {
            handle_extract_track(args, opts);
        }
        else if (action == g_action_patch)
        {
            handle_extract_patch(args, opts);
        }
        else if (action == g_action_dump_lcd)
        {
            handle_dump_lcd(args, opts);
        }
        else if (action == g_action_dump_wmd)
        {
            handle_dump_wmd(args, opts);
        }
        else if (action == g_action_dump_song)
        {
            handle_dump_song(args, opts);
        }
        else if (action == g_action_pack_data)
        {
            handle_pack_data(args, opts);
        }
        else if (!action.empty())
        {
            throw std::string("Unknown action '" + action + "' specified.");
        }
        else if (!opts.help && !opts.version)
        {
            throw std::string("No action specified.");
        }
    }
    // Report errors.
    catch (std::string &err)
    {
        fflush(stdout);
        fprintf(stderr, "%s: %s\n", PSXDMH_NAME, err.c_str());
        fflush(stderr);
        exit(1);
    }
    return 0;
}


namespace psxdmh
{


//
// Extract a range of songs.
//

static void
handle_extract_songs(const std::vector<std::string> &args, options &opts)
{
    // Default and validate the args.
    if (opts.sample_rate == 0)
    {
        opts.sample_rate = g_sample_rate_song;
    }
    validate_filters(opts);
    check_arg_count(args, 3, 4, args[0]);

    // Load the data files.
    wmd_file wmd;
    lcd_file lcd;
    load_music_dir(args[2], wmd, lcd, opts);
    if (opts.repair_patches)
    {
        lcd.repair_patches();
    }

    // Extract the songs.
    std::vector<uint16_t> ids;
    parse_range(args[1], (uint16_t) wmd.songs(), "song", ids);
    std::string name = args.size() >= 4 ? args[3] : "";
    if (!name.empty() && ids.size() != 1)
    {
        throw std::string("An output file name is only valid when a single song is being extracted.");
    }
    extract_songs(ids, wmd, lcd, name, opts);
}


//
// Extract one track from a song.
//

static void
handle_extract_track(const std::vector<std::string> &args, options &opts)
{
    // Default and validate the args.
    if (opts.sample_rate == 0)
    {
        opts.sample_rate = g_sample_rate_song;
    }
    validate_filters(opts);
    check_arg_count(args, 5, 5, args[0]);
    uint16_t song_index = (uint16_t) string_to_long(args[1], 0, SHRT_MAX, "song number");
    uint16_t track_index = (uint16_t) string_to_long(args[2], 0, SHRT_MAX, "track number");

    // Load the data files.
    wmd_file wmd;
    lcd_file lcd;
    load_music_dir(args[3], wmd, lcd, opts);
    if (opts.repair_patches)
    {
        lcd.repair_patches();
    }

    // Extract one track of a song.
    extract_track(song_index, track_index, wmd, lcd, args[4], opts);
}


//
// Extract a range of patches from an LCD file.
//

static void
handle_extract_patch(const std::vector<std::string> &args, options &opts)
{
    // Default and validate the args.
    if (opts.sample_rate == 0)
    {
        opts.sample_rate = g_sample_rate_patch;
    }
    check_arg_count(args, 3, 4, args[0]);

    // Load the data file.
    lcd_file lcd;
    load_lcd(args[2], lcd, opts);

    // Extract the patches.
    std::vector<uint16_t> ids;
    parse_range(args[1], lcd.maximum_patch_id() + 1, "patch", ids);
    std::string name = args.size() >= 4 ? args[3].c_str() : "";
    if (!name.empty() && ids.size() != 1)
    {
        throw std::string("An output file name is only valid when a single patch is being extracted.");
    }
    extract_patch(ids, lcd, name, opts);
}


//
// Dump the contents of an LCD file.
//

static void
handle_dump_lcd(const std::vector<std::string> &args, options &opts)
{
    // Validate the args.
    assert(!args.empty());
    assert(args[0] == g_action_dump_lcd);
    check_arg_count(args, 2, 2, args[0]);

    // Dump the LCD data.
    lcd_file lcd;
    load_lcd(args[1], lcd, opts);
    lcd.dump();
}


//
// Dump the contents of a WMD file.
//

static void
handle_dump_wmd(const std::vector<std::string> &args, options &opts)
{
    // Validate the args.
    assert(!args.empty());
    assert(args[0] == g_action_dump_wmd);
    check_arg_count(args, 2, 2, args[0]);

    // Dump the WMD data.
    wmd_file wmd;
    load_wmd(args[1], wmd, opts);
    wmd.dump(message::verbosity() == verbosity::verbose);
}


//
// Dump the details of a song from a WMD file.
//

static void
handle_dump_song(const std::vector<std::string> &args, options &opts)
{
    // Validate the args.
    assert(!args.empty());
    assert(args[0] == g_action_dump_song);
    check_arg_count(args, 3, 3, args[0]);

    // Dump details of a song.
    uint16_t song_index = (uint16_t) string_to_long(args[1], 0, SHRT_MAX, "song number");
    wmd_file wmd;
    load_wmd(args[2], wmd, opts);
    if (song_index >= wmd.songs())
    {
        throw std::string("Invalid song number.");
    }
    wmd.song(song_index).dump(wmd, song_index, message::verbosity() == verbosity::verbose);
}


//
// Merge the contents of LCD files.
//

static void
handle_pack_data(const std::vector<std::string> &args, options &opts)
{
    // Validate the args.
    assert(!args.empty());
    assert(args[0] == g_action_pack_data);
    check_arg_count(args, 3, 3, args[0]);

    // Load the data files.
    wmd_file wmd;
    lcd_file lcd;
    load_music_dir(args[1], wmd, lcd, opts);

    // Write a merged LCD file.
    message::writef(verbosity::normal, "Creating LCD file: %s\n", args[2].c_str());
    lcd.sort();
    lcd.write(args[2]);
}


//
// Display version and license information.
//

static void
show_version()
{
    std::string license = "psxdmh is free software: you can redistribute it "
        "and/or modify it under the terms of the GNU General Public License as "
        "published by the Free Software Foundation, either version 3 of the "
        "License, or (at your option) any later version.  psxdmh is "
        "distributed in the hope that it will be useful, but without any "
        "warranty; without even the implied warranty of merchantability or "
        "fitness for a particular purpose.  See the GNU General Public License "
        "for more details.  You should have received a copy of the GNU General "
        "Public License along with psxdmh.  If not, see <www.gnu.org/licenses>.";
    std::string source = "The psxdmh source code can be downloaded from "
        "<" PSXDMH_URL ">.";
    printf("%s %s\n\n", PSXDMH_NAME, PSXDMH_VERSION_STRING);
    printf("%s\n\n", word_wrap(license, 0, 80).c_str());
    printf("%s\n\n", word_wrap(source, 0, 80).c_str());
}


//
// Display usage information.
//

static void
show_help()
{
    printf("\n%s %s: PlayStation Doom Music Hack\n%s\n\n", PSXDMH_NAME, PSXDMH_VERSION_STRING, PSXDMH_COPYRIGHT);

    std::string what = PSXDMH_NAME " can perform several actions related to extracting sounds and music from the PlayStation versions of Doom and Final Doom.  "
        "The following actions are available:";
    printf("%s\n\n", word_wrap(what, 0, 80).c_str());

    std::string usage_song = "Extract one or more songs into WAV files.  "
        "The songs can be specified as a series of one or more individual numbers or hyphen-separated ranges delimited by commas.  "
        "The WMD and LCD data files must be in <music_dir>.  "
        "Files are collected recursively.  "
        "If a single song is being extracted then an output file name can optionally be specified, otherwise file names will be generated automatically.";
    printf(PSXDMH_NAME " [options] song <song_indexes> <music_dir> [<wav_file>]\n%s\n\n", word_wrap(usage_song, 4, 80).c_str());

    std::string usage_track = "Extract a single track from a song into a WAV file.  "
        "The WMD and LCD data files must be in <music_dir>.  "
        "Files are collected recursively.";
    printf(PSXDMH_NAME " [options] track <song_index> <track_index> <music_dir> <wav_file>\n%s\n\n", word_wrap(usage_track, 4, 80).c_str());

    std::string usage_patch = "Extract one or more patches into WAV files.  "
        "The patches can be specified as a series of one or more individual numbers or hyphen-separated ranges delimited by commas.  "
        "An LCD file can be specified with <lcd_file>, or it can refer to a directory containing data files.  "
        "If a single patch is being extracted then an output file name can optionally be specified, otherwise file names will be generated automatically.  "
        "Note that the only audio-related option that affects this action is --play-count.";
    printf(PSXDMH_NAME " [options] patch <patch_ids> <lcd_file> [<wav_file>]\n%s\n\n", word_wrap(usage_patch, 4, 80).c_str());

    std::string usage_dump_lcd = "Dump details about the contents of an LCD file.  "
        "An LCD file can be specified with <lcd_file>, or it can refer to a directory containing data files.";
    printf(PSXDMH_NAME " [options] dump-lcd <lcd_file>\n%s\n\n", word_wrap(usage_dump_lcd, 4, 80).c_str());

    std::string usage_dump_wmd = "Dump details about the contents of a WMD file.  "
        "A WMD file can be specified with <wmd_file>, or it can refer to a directory containing data files.";
    printf(PSXDMH_NAME " [options] dump-wmd <wmd_file>\n%s\n\n", word_wrap(usage_dump_wmd, 4, 80).c_str());

    std::string usage_dump_song = "Dump details about a song from a WMD file.  "
        "A WMD file can be specified with <wmd_file>, or it can refer to a directory containing data files.";
    printf(PSXDMH_NAME " [options] dump-song <song_index> <wmd_file>\n%s\n\n", word_wrap(usage_dump_song, 4, 80).c_str());

    std::string usage_pack_data = "Merge the contents of multiple LCD files from <music_dir>, and write the result into <new_lcd_file>.";
    printf(PSXDMH_NAME " [options] pack-data <music_dir> <new_lcd_file>\n%s\n\n", word_wrap(usage_pack_data, 4, 80).c_str());

    printf("Options:\n\n%s\n", options().describe().c_str());

    printf("Report bugs to: " PSXDMH_EMAIL "\n" PSXDMH_NAME " home page: <" PSXDMH_URL ">\n\n");
}


//
// Load LCD data.
//

static void
load_lcd(std::string file_name, lcd_file &lcd, const options &opts)
{
    if (type_of_file(file_name) == file_type::file)
    {
        lcd.parse(file_name);
    }
    else
    {
        wmd_file wmd;
        load_music_dir(file_name, wmd, lcd, opts);
    }
}


//
// Load WMD data.
//

static void
load_wmd(std::string file_name, wmd_file &wmd, const options &opts)
{
    if (type_of_file(file_name) == file_type::file)
    {
        wmd.parse(file_name);
    }
    else
    {
        lcd_file lcd;
        load_music_dir(file_name, wmd, lcd, opts);
    }
}


//
// Load data files from a music directory recursively.
//

static void
load_music_dir(std::string music_dir, wmd_file &wmd, lcd_file &lcd, const options &opts, bool root)
{
    // Enumerate the contents of the directory.
    enum_dir iter(music_dir);
    std::string name;
    file_type type;
    while (iter.next(name, type))
    {
        // Recursively enumerate directories.
        std::string full_name = combine_paths(music_dir, name);
        if (type == file_type::directory)
        {
            load_music_dir(full_name, wmd, lcd, opts, false);
        }
        // Look for WMD and LCD files.
        else if (type == file_type::file)
        {
            // Handle WMD files. There can be only one.
            std::string suffix = name.length() > 4 ? name.substr(name.length() - 4) : "";
            if (strcasecmp(suffix.c_str(), ".wmd") == 0)
            {
                if (!wmd.is_empty())
                {
                    throw std::string("Found more than one WMD file. Only one is allowed.");
                }
                message::writef(verbosity::verbose, "Loading '%s'.\n", name.c_str());
                wmd.parse(full_name);
            }
            // Accumulate LCD files.
            else if (strcasecmp(suffix.c_str(), ".lcd") == 0)
            {
                // If the destination LCD file is empty then parse into it.
                message::writef(verbosity::verbose, "Loading '%s'.\n", name.c_str());
                if (lcd.is_empty())
                {
                    lcd.parse(full_name);
                }
                // Otherwise parse into a temporary then merge it in.
                else
                {
                    lcd_file temp(full_name);
                    lcd.merge(temp);
                }
            }
        }
    }

    // Report errors.
    if (root && wmd.is_empty())
    {
        throw std::string("No WMD file found.");
    }
    if (root && lcd.is_empty())
    {
        throw std::string("No LCD files found.");
    }
}


//
// Validate the filter values set in the options.
//

static void
validate_filters(const options &opts)
{
    // Validate the low and high filter frequencies.
    if (opts.high_pass >= opts.sample_rate / 2)
    {
        throw std::string("The high-pass filter frequency must be less than half the sample rate.");
    }
    if (opts.low_pass >= opts.sample_rate / 2)
    {
        throw std::string("The low-pass filter frequency must be less than half the sample rate.");
    }
    if (opts.high_pass != 0 && opts.low_pass != 0 && opts.high_pass >= opts.low_pass)
    {
        throw std::string("The high-pass filter frequency must be less than the low-pass filter frequency.");
    }
}


//
// Check if the number of arguments falls into a range. If not, a descriptive
// std::string will be thrown.
//

static void
check_arg_count(const std::vector<std::string> &args, size_t min_args, size_t max_args, std::string what)
{
    assert(min_args <= max_args);
    if (args.size() < min_args || args.size() > max_args)
    {
        throw std::string("Invalid number of arguments for '") + what + "'.";
    }
}


}; //namespace psxdmh
