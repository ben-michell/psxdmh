// psxdmh/src/wmd_file.cpp
// Parsing of WMD format files.
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

#include "channel.h"
#include "envelope.h"
#include "music_stream.h"
#include "safe_file.h"
#include "utility.h"
#include "wmd_file.h"


namespace psxdmh
{


// Signature identifying SPSX files ("SPSX").
#define PSXDMH_SPSX_SIGNATURE       (0x58535053)

// SPSX version supported.
#define PSXDMH_SPSX_VERSION         (1)


// Raw instrument definition as it appears in the WMD file.
struct wmd_raw_instrument
{
    // Number of sub-instruments used by the instrument.
    uint16_t sub_instruments;

    // Index of the first sub-instrument used by the instrument.
    uint16_t first_sub_instrument;
};


//
// Construction.
//

wmd_file::wmd_file(std::string file_name)
{
    parse(file_name);
}


//
// Get a track from a song by index.
//

const wmd_song_track &
wmd_file::track(size_t song_index, size_t track_index) const
{
    assert(song_index < m_songs.size());
    assert(track_index < m_songs[song_index].tracks.size());
    return m_songs[song_index].tracks[track_index];
}


//
// Convert a raw note value to a frequency, taking into account tuning and pitch
// bending.
//

uint32_t
wmd_file::note_to_frequency(size_t instrument_index, uint8_t note, mono_t unit_pitch_bend) const
{
    // Adjust the note for the tuning and pitch bend.
    const wmd_sub_instrument &sub_instrument = instrument(instrument_index).sub_instrument(note);
    assert(sub_instrument.bend_sensitivity_down == sub_instrument.bend_sensitivity_up);
    double tuning = sub_instrument.tuning + double(sub_instrument.fine_tuning) / 256;
    double adjusted_note = (note - tuning) / 12.0 + sub_instrument.bend_sensitivity_down * unit_pitch_bend;

    // Map the note to a frequency.
    return (uint32_t) std::max(1, int32_t(44100.0 * pow(2.0, adjusted_note) + 0.5));
}


//
// Load from a file.
//

void
wmd_file::parse(std::string file_name)
{
    // The file must start with the signature "SPSX" and a version of 1.
    m_songs.clear();
    m_instruments.clear();
    safe_file file(file_name, file_mode::read);
    if (file.read_32_le() != PSXDMH_SPSX_SIGNATURE)
    {
        throw std::string("Not a WMD file (bad signature).");
    }
    if (file.read_32_le() != PSXDMH_SPSX_VERSION)
    {
        throw std::string("WMD file uses an unsupported SPSX version.");
    }

    // Get the number of songs.
    size_t song_count = file.read_16_le();

    // Skip 14 bytes of unknown purpose.
    file.read(m_unknown_0, sizeof(m_unknown_0));

    // Get the number and size of the instrument records.
    size_t instrument_count = (size_t) file.read_16_le();
    if (file.read_16_le() != 4)
    {
        throw std::string("Corrupt WMD file (bad instrument record size).");
    }

    // Get the number and size of the sub-instrument records.
    size_t sub_instrument_count = (size_t) file.read_16_le();
    if (file.read_16_le() != 16)
    {
        throw std::string("Corrupt WMD file (bad sub-instrument record size).");
    }

    // Get the number and size of the patch records.
    size_t patch_count = (size_t) file.read_16_le();
    if (file.read_16_le() != 12)
    {
        throw std::string("Corrupt WMD file (bad patch record size).");
    }

    // Skip 8 bytes of unknown purpose.
    file.read(m_unknown_1, sizeof(m_unknown_1));

    // Read the instrument definitions. This data needs to be held temporarily
    // until the sub-instruments have been read.
    std::vector<wmd_raw_instrument> raw_instruments;
    raw_instruments.resize(instrument_count);
    size_t expected_first = 0;
    size_t index;
    for (index = 0; index < instrument_count; ++index)
    {
        raw_instruments[index].sub_instruments = file.read_16_le();
        raw_instruments[index].first_sub_instrument = file.read_16_le();
        if (raw_instruments[index].first_sub_instrument != expected_first)
        {
            throw std::string("Corrupt WMD file (non-contiguous sub-instruments).");
        }
        expected_first += raw_instruments[index].sub_instruments;
    }
    if (expected_first != sub_instrument_count)
    {
        throw std::string("Corrupt WMD file (wrong number of sub-instruments).");
    }

    // Read the sub-instrument definitions. Since they are in instrument order
    // and are contiguous we can read them directly into the instrument vector.
    m_instruments.resize(instrument_count);
    for (index = 0; index < instrument_count; ++index)
    {
        m_instruments[index].sub_instruments.resize(raw_instruments[index].sub_instruments);
        for (size_t sub = 0; sub < raw_instruments[index].sub_instruments; ++sub)
        {
            wmd_sub_instrument &sub_instrument = m_instruments[index].sub_instruments[sub];
            sub_instrument.priority = file.read_8();
            sub_instrument.flags = file.read_8();
            sub_instrument.volume = file.read_8();
            sub_instrument.pan = file.read_8();
            sub_instrument.tuning = file.read_8();
            sub_instrument.fine_tuning = file.read_8();
            sub_instrument.first_note = file.read_8();
            sub_instrument.last_note = file.read_8();
            sub_instrument.bend_sensitivity_down = file.read_8();
            sub_instrument.bend_sensitivity_up = file.read_8();
            sub_instrument.patch = file.read_16_le();
            sub_instrument.spu_ads = file.read_16_le();
            sub_instrument.spu_sr = file.read_16_le();
        }
    }

    // Skip the patch array. Each record consists of what appears to be an
    // offset relating to where the ADPCM encoded patch data should be loaded,
    // the length of the patch data, and a field that is always 0. Given that
    // the patch data can be read without these details, there's no need to
    // store them.
    file.seek(file.tell() + patch_count * (4 + 4 + 4));

    // Read the song and track definitions.
    m_songs.resize(song_count);
    for (index = 0; index < song_count; ++index)
    {
        // Prepare the new song and read the header.
        wmd_song &song = m_songs[index];
        size_t tracks_in_song = file.read_16_le();
        song.tracks.reserve(tracks_in_song);
        file.read(song.unknown, sizeof(song.unknown));

        // Read the tracks.
        song.tracks.resize(tracks_in_song);
        for (size_t track_index = 0; track_index < tracks_in_song; ++track_index)
        {
            // Read the track fields.
            wmd_song_track &track = song.tracks[track_index];
            file.read(track.unknown_0, sizeof(track.unknown_0));
            track.instrument = file.read_16_le();
            file.read(track.unknown_1, sizeof(track.unknown_1));
            track.beats_per_minute = file.read_16_le();
            track.ticks_per_beat = file.read_16_le();
            track.repeat = file.read_16_le() != 0;
            uint32_t data_length = file.read_32_le();
            track.repeat_start = track.repeat ? file.read_32_le() : 0;

            // Store a pointer to the music data and skip over it.
            track.data.resize(data_length);
            file.read(track.data.data(), data_length);
        }
    }
}


//
// Store the contents of this object in a file.
//

void
wmd_file::write(std::string file_name) const
{
    // Count the number of sub-instruments.
    assert(!is_empty());
    size_t sub_instruments = 0;
    std::vector<wmd_instrument>::const_iterator instr_iter;
    for (instr_iter = m_instruments.cbegin(); instr_iter != m_instruments.cend(); ++instr_iter)
    {
        sub_instruments += instr_iter->sub_instruments.size();
    }

    // Write the file.
    safe_file wmd_file(file_name.c_str(), file_mode::write);
    wmd_file.write_32_le(PSXDMH_SPSX_SIGNATURE);
    wmd_file.write_32_le(PSXDMH_SPSX_VERSION);
    wmd_file.write_16_le((uint16_t) songs());
    wmd_file.write(unknown_0(), unknown_0_size());
    wmd_file.write_16_le((uint16_t) instruments());
    wmd_file.write_16_le(4);
    wmd_file.write_16_le((uint16_t) sub_instruments);
    wmd_file.write_16_le(16);
    wmd_file.write_16_le(0);
    wmd_file.write_16_le(12);
    wmd_file.write(unknown_1(), unknown_1_size());
    uint16_t first_sub_instrument = 0;
    for (instr_iter = m_instruments.cbegin(); instr_iter != m_instruments.cend(); ++instr_iter)
    {
        wmd_file.write_16_le((uint16_t) instr_iter->sub_instruments.size());
        wmd_file.write_16_le(first_sub_instrument);
        first_sub_instrument += (uint16_t) instr_iter->sub_instruments.size();
    }
    assert(first_sub_instrument == sub_instruments);
    for (instr_iter = m_instruments.cbegin(); instr_iter != m_instruments.cend(); ++instr_iter)
    {
        for (auto sub_iter = instr_iter->sub_instruments.cbegin(); sub_iter != instr_iter->sub_instruments.cend(); ++sub_iter)
        {
            wmd_file.write_8(sub_iter->priority);
            wmd_file.write_8(sub_iter->flags);
            wmd_file.write_8(sub_iter->volume);
            wmd_file.write_8(sub_iter->pan);
            wmd_file.write_8(sub_iter->tuning);
            wmd_file.write_8(sub_iter->fine_tuning);
            wmd_file.write_8(sub_iter->first_note);
            wmd_file.write_8(sub_iter->last_note);
            wmd_file.write_8(sub_iter->bend_sensitivity_down);
            wmd_file.write_8(sub_iter->bend_sensitivity_up);
            wmd_file.write_16_le(sub_iter->patch);
            wmd_file.write_16_le(sub_iter->spu_ads);
            wmd_file.write_16_le(sub_iter->spu_sr);
        }
    }
    for (auto song_iter = m_songs.cbegin(); song_iter != m_songs.cend(); ++song_iter)
    {
        wmd_file.write_16_le((uint16_t) song_iter->tracks.size());
        wmd_file.write(song_iter->unknown, sizeof(song_iter->unknown));
        for (auto track_iter = song_iter->tracks.cbegin(); track_iter != song_iter->tracks.cend(); ++track_iter)
        {
            wmd_file.write(track_iter->unknown_0, sizeof(track_iter->unknown_0));
            wmd_file.write_16_le(track_iter->instrument);
            wmd_file.write(track_iter->unknown_1, sizeof(track_iter->unknown_1));
            wmd_file.write_16_le(track_iter->beats_per_minute);
            wmd_file.write_16_le(track_iter->ticks_per_beat);
            wmd_file.write_16_le(track_iter->repeat ? 1 : 0);
            wmd_file.write_32_le((uint32_t) track_iter->data.size());
            if (track_iter->repeat)
            {
                wmd_file.write_32_le(track_iter->repeat_start);
            }
            wmd_file.write(track_iter->data.data(), track_iter->data.size());
        }
    }
    wmd_file.close();
}


//
// Dump details about the WMD file.
//

void
wmd_file::dump(bool detailed) const
{
    // Dump the instruments.
    printf("WMD contains %d instruments:\n\n", (int) instruments());
    size_t index;
    for (index = 0; index < instruments(); ++index)
    {
        instrument(index).dump(index, "  ", detailed);
        printf("\n");
    }

    // Dump brief information about the songs.
    printf("\nWMD contains %d songs:\n\n", (int) songs());
    for (index = 0; index < songs(); ++index)
    {
        song(index).dump_header(index, "  ");
        printf("\n");
    }

    // Dump the unknown bytes.
    printf("\nUnknown bytes (group 1: %zu bytes following file header):\n", unknown_0_size());
    printf("  %s\n", hex_bytes(unknown_0(), unknown_0_size()).c_str());
    printf("\nUnknown bytes (group 2: %zu bytes following patch record count and size):\n", unknown_1_size());
    printf("  %s\n", hex_bytes(unknown_1(), unknown_1_size()).c_str());
    printf("\n");
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Dump details about the song.
//

void
wmd_song::dump(const wmd_file &wmd, size_t id, bool detailed) const
{
    // Remember which instruments are used so we can dump their details too.
    std::vector<bool> instrument_used(wmd.instruments(), false);

    // Create the music streams. Use a tick rate the same as the first track, as
    // all Doom and Final Doom songs use the same rate for all tracks in a given
    // song (validate this just in case). This allows the extraction of music
    // events with maximum precision.
    if (tracks.empty())
    {
        throw std::string("Corrupt song data: song contains no tracks.");
    }
    uint32_t ticks_per_minute = uint32_t(tracks[0].beats_per_minute) * uint32_t(tracks[0].ticks_per_beat);
    std::vector<std::unique_ptr<music_stream>> streams;
    size_t index;
    for (index = 0; index < tracks.size(); ++index)
    {
        streams.push_back(std::unique_ptr<music_stream>(new music_stream(tracks[index], ticks_per_minute)));
        if (tracks[index].instrument >= wmd.instruments())
        {
            throw std::string("Invalid instrument number in track header.");
        }
        if (tracks[index].beats_per_minute != tracks[0].beats_per_minute
            || tracks[index].ticks_per_beat != tracks[0].ticks_per_beat)
        {
            throw std::string("Tracks use different tick rates (unsupported).");
        }
        instrument_used[tracks[index].instrument] = true;
    }

    // Dump the contents of the song.
    dump_header(id, "");
    printf("\n");
    printf(" Time     ");
    std::string underline = "-----------";
    for (index = 0; index < streams.size(); ++index)
    {
        printf(" | Track #%d        ", (int) index);
        underline += "+------------------";
    }
    printf("\n");
    printf("%s\n", underline.c_str());

    // Track the lowest and highest frequencies used on each track. This also
    // requires the tracking of pitch bends. This isn't quite perfect, as it
    // will not be able to track the pitch bending of notes after they've been
    // released but before they've finished playing. A very minor flaw.
    std::vector<std::pair<int32_t, int32_t>> ranges;
    std::vector<std::vector<bool>> notes;
    notes.resize(tracks.size());
    std::vector<mono_t> unit_pitch_bends;
    for (index = 0; index < tracks.size(); ++index)
    {
        ranges.push_back(std::make_pair(-1, -1));
        notes[index].resize(128);
        unit_pitch_bends.push_back(0.0);
    }

    // Scan through the song, printing events as they are found. Note that more
    // than one event may occur at the same time.
    uint32_t ticks = 0;
    uint32_t last_ticks = 0xffffffffU;
    size_t running_streams = streams.size();
    std::string line;
    char temp[32];
    music_event event;
    while (running_streams != 0)
    {
        // Check if any events are available at this time.
        auto predicate = [](const std::unique_ptr<music_stream> &stream) { return stream->have_event(); };
        if (std::any_of(streams.cbegin(), streams.cend(), predicate))
        {
            line = "";
            for (index = 0; index < streams.size(); ++index)
            {
                if (streams[index]->get_event(event))
                {
                    switch (event.code)
                    {
                    case music_event_code::note_on:
                        sprintf(temp, "On $%02x @%d%%", event.data_0, 100 * event.data_1 / 0x7f);
                        notes[index][event.data_0] = true;
                        break;

                    case music_event_code::note_off:
                        sprintf(temp, "Off $%02x", event.data_0);
                        notes[index][event.data_0] = false;
                        break;

                    case music_event_code::set_instrument:
                        sprintf(temp, "Instrument %d", event.data_0);
                        if (event.data_0 < 0 || event.data_0 >= (int32_t) wmd.instruments())
                        {
                            throw std::string("Invalid instrument number in music stream.");
                        }
                        instrument_used[event.data_0] = true;
                        break;

                    case music_event_code::pitch_bend:
                        if (event.data_0 != 0)
                        {
                            sprintf(temp, "Bend %.1lf%%", 100.0 * double(event.data_0) / double(0x2000));
                        }
                        else
                        {
                            strcpy(temp, "Bend off");
                        }
                        unit_pitch_bends[index] = mono_t(event.data_0) / 0x2000 / 12;
                        break;

                    case music_event_code::volume:
                        sprintf(temp, "Volume %d%%", 100 * event.data_0 / 0x7f);
                        break;

                    case music_event_code::pan_offset:
                        sprintf(temp, "Pan offset $%02x", event.data_0);
                        break;

                    case music_event_code::set_marker:
                        sprintf(temp, "Mark @%d", event.data_0);
                        break;

                    case music_event_code::jump_to_marker:
                        sprintf(temp, "Jump to #%d", event.data_0);
                        break;

                    case music_event_code::unknown_0b:
                        sprintf(temp, "Unknown 0B $%02x", event.data_0);
                        break;

                    case music_event_code::unknown_0e:
                        sprintf(temp, "Unknown 0E $%02x", event.data_0);
                        break;

                    case music_event_code::eos:
                        strcpy(temp, "End");
                        assert(running_streams > 0);
                        running_streams--;
                        break;

                    default:
                        assert(!"Unhandled event code.");
                        strcpy(temp, "<Unhandled> ");
                        break;
                    }
                    assert(strlen(temp) <= 14);
                    strcat(temp, &"              "[strlen(temp)]);
                    line += " | ";
                    line += temp;
                }
                else
                {
                    line += " |               ";
                }

                // Update the frequency limits and flag where the SPU limit is
                // being exceeded.
                bool exceeding = check_note_frequencies(wmd, tracks[index].instrument, notes[index], ranges[index], unit_pitch_bends[index]);
                line += exceeding ? " *" : "  ";
            }

            // Print the line. The time is only printed once for any given time.
            if (ticks != last_ticks)
            {
                uint32_t milliseconds = uint32_t(double(ticks) / ticks_per_minute * 60.0 * 1000.0);
                uint32_t seconds = milliseconds / 1000;
                milliseconds -= seconds * 1000;
                uint32_t minutes = seconds / 60;
                seconds -= minutes * 60;
                printf("%3d:%02d.%03d%s\n", minutes, seconds, milliseconds, line.c_str());
                last_ticks = ticks;
            }
            else
            {
                printf("          %s\n", line.c_str());
            }
        }
        // The time is advanced only when no events were available.
        else
        {
            std::for_each(streams.begin(), streams.end(), [](const std::unique_ptr<music_stream> &stream) { stream->tick(); });
            ticks++;
        }
    }

    // Finish off the song.
    printf("%s\n\n", underline.c_str());

    // Print the range of playback frequencies for each track.
    printf("Playback frequency ranges:\n");
    bool limit_exceeded = false;
    for (index = 0; index < tracks.size(); ++index)
    {
        std::string star;
        if (ranges[index].second > (int32_t) channel::spu_max_frequency())
        {
            limit_exceeded = true;
            star = " *";
        }
        printf("  Track #%d: %d Hz to %d Hz%s\n", (int) index, ranges[index].first, ranges[index].second, star.c_str());
    }
    if (limit_exceeded)
    {
        printf("  * Note: Real PSX hardware is limited to a maximum frequency of %u Hz.\n", channel::spu_max_frequency());
    }
    printf("\n");

    // Print details of the instruments used.
    for (index = 0; index < wmd.instruments(); ++index)
    {
        if (instrument_used[index])
        {
            wmd.instrument(index).dump(index, "", detailed);
            printf("\n");
        }
    }
}


//
// Dump the header details of the song.
//

void
wmd_song::dump_header(size_t id, std::string indent) const
{
    // Print details of the song.
    printf("%sSong %d ($%02x):\n", indent.c_str(), (int) id, (int) id);
    printf("%s  Unknown bytes: %s\n", indent.c_str(), hex_bytes(unknown, sizeof(unknown)).c_str());
    for (size_t track_index = 0; track_index < tracks.size(); ++track_index)
    {
        const wmd_song_track &track = tracks[track_index];
        printf("%s  Track %d:\n", indent.c_str(), (int) track_index);
        printf("%s    Unknown bytes: %s\n", indent.c_str(), hex_bytes(track.unknown_0, sizeof(track.unknown_0)).c_str());
        printf("%s    Instrument: %u ($%02x)\n", indent.c_str(), track.instrument, track.instrument);
        printf("%s    Unknown bytes: %s\n", indent.c_str(), hex_bytes(track.unknown_1, sizeof(track.unknown_1)).c_str());
        printf("%s    Beats per minute: %u\n", indent.c_str(), track.beats_per_minute);
        printf("%s    Ticks per beat: %u\n", indent.c_str(), track.ticks_per_beat);
        if (track.repeat)
        {
            printf("%s    Repeat start: %u\n", indent.c_str(), track.repeat_start);
        }
        else
        {
            printf("%s    No repeat.\n", indent.c_str());
        }
    }
}


//
// Update the playback frequency range for a set of notes.
//

bool
wmd_song::check_note_frequencies(const wmd_file &wmd, uint16_t instrument, const std::vector<bool> &notes, std::pair<int32_t, int32_t> &range, mono_t unit_pitch_bend)
{
    assert(notes.size() == 128);
    bool exceeding = false;
    for (uint8_t note_index = 0; note_index < (uint8_t) notes.size(); ++note_index)
    {
        if (notes[note_index])
        {
            int32_t frequency = (int32_t) wmd.note_to_frequency(instrument, note_index, unit_pitch_bend);
            if (range.first < 0 || frequency < range.first)
            {
                range.first = frequency;
            }
            if (range.second < 0 || frequency > range.second)
            {
                range.second = frequency;
            }
            if (frequency > (int32_t) channel::spu_max_frequency())
            {
                exceeding = true;
            }
        }
    }
    return exceeding;
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Get a sub-instrument by note.
//

const wmd_sub_instrument &
wmd_instrument::sub_instrument(uint8_t note) const
{
    assert(note < 0x80);
    auto predicate = [note](const wmd_sub_instrument &sub) { return note >= sub.first_note && note <= sub.last_note; };
    auto sub = std::find_if(sub_instruments.cbegin(), sub_instruments.cend(), predicate);
    if (sub != sub_instruments.cend())
    {
        return *sub;
    }
    throw std::string("Missing a sub-instrument for note $") + hex_byte(note) + ".";
}


//
// Dump details about the instrument.
//

void
wmd_instrument::dump(size_t id, std::string indent, bool detailed) const
{
    // Print details of the instrument.
    printf("%sInstrument %d ($%02x):\n", indent.c_str(), (int) id, (int) id);
    for (size_t sub_index = 0; sub_index < sub_instruments.size(); ++sub_index)
    {
        const wmd_sub_instrument &sub = sub_instruments[sub_index];
        printf("%s  Sub-instrument %d:\n", indent.c_str(), (int) sub_index);
        printf("%s    Notes: %u-%u ($%02x-$%02x)\n", indent.c_str(), sub.first_note, sub.last_note, sub.first_note, sub.last_note);
        printf("%s    Patch: %u ($%02x)\n", indent.c_str(), sub.patch, sub.patch);
        printf("%s    Volume: %u ($%02x)\n", indent.c_str(), sub.volume, sub.volume);
        printf("%s    Tuning: %u ($%02x)\n", indent.c_str(), sub.tuning, sub.tuning);
        printf("%s    Fine tuning: %u ($%02x)\n", indent.c_str(), sub.fine_tuning, sub.fine_tuning);
        printf("%s    Pan: %u ($%02x)\n", indent.c_str(), sub.pan, sub.pan);
        printf("%s    Pitch bend sensitivity: %u / %u ($%02x / $%02x)\n", indent.c_str(), sub.bend_sensitivity_up, sub.bend_sensitivity_down, sub.bend_sensitivity_up, sub.bend_sensitivity_down);
        printf("%s    Flags: $%02x\n", indent.c_str(), sub.flags);
        printf("%s    Priority: %u ($%02x)\n", indent.c_str(), sub.priority, sub.priority);
        printf("%s    SPU registers (ADS/SR): $%04x / $%04x\n", indent.c_str(), sub.spu_ads, sub.spu_sr);
        if (detailed)
        {
            envelope(sub.spu_ads, sub.spu_sr).dump(indent + "      ");
        }
    }
}


}; //namespace psxdmh
