// psxdmh/src/wmd_file.h
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

#ifndef PSXDMH_SRC_WMD_FILE_H
#define PSXDMH_SRC_WMD_FILE_H


#include "sample.h"


namespace psxdmh
{


// Forwards.
struct wmd_instrument;
struct wmd_song;
struct wmd_sub_instrument;
struct wmd_song_track;


// WMD file parser. All errors are reported by a thrown std::string.
class wmd_file
{
public:

    // Construction. Note that the constructor will throw an exception if it
    // encounters an error.
    wmd_file() {}
    wmd_file(std::string file_name);

    // Test if the file is empty.
    bool is_empty() const { return m_songs.empty() && m_instruments.empty(); }

    // Number of songs.
    size_t songs() const { return m_songs.size(); }

    // Get a song by index.
    const wmd_song &song(size_t index) const { assert(index < m_songs.size()); return m_songs[index]; }

    // Get a track from a song by index.
    const wmd_song_track &track(size_t song_index, size_t track_index) const;

    // Number of instruments.
    size_t instruments() const { return m_instruments.size(); }

    // Get an instrument by index.
    const wmd_instrument &instrument(size_t index) const { assert(index < m_instruments.size()); return m_instruments[index]; }

    // Convert a raw note value to a frequency, taking into account tuning and
    // pitch bending.
    uint32_t note_to_frequency(size_t instrument_index, uint8_t note, mono_t unit_pitch_bend) const;

    // Access to the bytes of unknown purpose.
    const uint8_t *unknown_0() const { return m_unknown_0; }
    size_t unknown_0_size() const { return sizeof(m_unknown_0); }
    const uint8_t *unknown_1() const { return m_unknown_1; }
    size_t unknown_1_size() const { return sizeof(m_unknown_1); }

    // Load from a file. The current contents of this object are overwritten.
    void parse(std::string file_name);

    // Store the contents of this object in a file.
    void write(std::string file_name) const;

    // Dump details about the WMD file.
    void dump(bool detailed) const;

private:

    // Instruments.
    std::vector<wmd_instrument> m_instruments;

    // Songs.
    std::vector<wmd_song> m_songs;

    // Unknown bytes following the file header.
    uint8_t m_unknown_0[14];

    // Unknown bytes following the patch record count and size.
    uint8_t m_unknown_1[8];
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Tracks contain song data for a single instrument.
struct wmd_song_track
{
    // Index of the instrument used for the track.
    uint16_t instrument;

    // Tempo specification.
    uint16_t beats_per_minute;
    uint16_t ticks_per_beat;

    // Flag indicating whether the track repeats. Sound effects don't and music
    // does, with the exception of two songs in Final Doom: 117 and 118.
    bool repeat;

    // Offset to the start of the start of the repeating part of the track.
    // Tracks without a repeat do not include this field.
    uint32_t repeat_start;

    // Music data encoded in a MIDI-like form. The music_stream class is used to
    // parse this data.
    std::vector<uint8_t> data;

    // Unknown bytes at the start of the track header. All songs have 01 18 80
    // 00 01 28, and all sound effects have 01 01 64 00 00 28. The second byte
    // may relate to number of voices: all sound effects are 1 and all songs are
    // 24 (the number of SPU channels). The third byte may be a priority as it
    // matches the priority for song and sound effect sub-instruments.
    uint8_t unknown_0[6];

    // Unknown bytes following the patch index. Indexes 2 and 3 may be volume
    // and pan respectively. All songs and sound effects have 00 00 7f 40 00 00.
    uint8_t unknown_1[6];
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Songs are a collection of one or more tracks.
struct wmd_song
{
    // Dump details about the song.
    void dump(const wmd_file &wmd, size_t id, bool detailed) const;

    // Dump the header details of the song.
    void dump_header(size_t id, std::string indent) const;

    // Update the playback frequency range for a set of notes. The return value
    // is true if the currently playing notes exceed the SPU limit, false
    // otherwise.
    static bool check_note_frequencies(const wmd_file &wmd, uint16_t instrument, const std::vector<bool> &notes, std::pair<int32_t, int32_t> &range, mono_t unit_pitch_bend);

    // Collection of tracks for the song.
    std::vector<wmd_song_track> tracks;

    // Unknown bytes following the group size. Seems to be 00 00 for sound
    // effects, and 47 00 for songs except song 95 which is 6d 82.
    uint8_t unknown[2];
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Sub-instruments manage a subset of the range of notes for an instrument.
struct wmd_sub_instrument
{
    // Range of notes this sub-instrument applies to (inclusive of the end
    // points).
    uint8_t first_note;
    uint8_t last_note;

    // Patch index used by the sub-instrument.
    uint16_t patch;

    // Volume adjustment for this sub-instrument.
    uint8_t volume;

    // Note number that maps to the natural playback frequency of 44100 Hz.
    uint8_t tuning;

    // Fine tuning. Fractional note number added to tuning above after dividing
    // by 256.
    uint8_t fine_tuning;

    // Panning. Full left is 0x00, centre is 0x40, full right is 0x7f.
    uint8_t pan;

    // Pitch bend sensitivity. These values give the number of notes to shift a
    // note by at full bend deflection (+/- 0x2000). A value of zero means pitch
    // bending has no effect. Although I can't tell which value is down and
    // which is up it doesn't actually matter since all sub-instruments used by
    // both Doom and Final Doom use identical pairs of numbers.
    uint8_t bend_sensitivity_down;
    uint8_t bend_sensitivity_up;

    // This appears to be a flag. The only value used is 0x80, which is not set
    // for any sound effect, but is set for all songs except instrument/patch
    // 91/88 (song 90), 94/90 (song 91), 111/106 (song 99).
    uint8_t flags;

    // Priority of the sound. Presumably this is used to manage the allocation
    // of the limited number of channels on the SPU. This is not relevant for
    // psxdmh since an unlimited number of channels are available. All sound
    // effects have this set to 0x64, while Doom song instruments all use 0x80
    // and all Final Doom-only song instruments use 0x00.
    uint8_t priority;

    // SPU attack/decay/sustain level register setting.
    uint16_t spu_ads;

    // SPU sustain/release level register setting.
    uint16_t spu_sr;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Instruments are a collection of one or more sub-instruments.
struct wmd_instrument
{
    // Get a sub-instrument by note.
    const wmd_sub_instrument &sub_instrument(uint8_t note) const;

    // Dump details about the instrument.
    void dump(size_t id, std::string indent, bool detailed) const;

    // Collection of sub-instruments for the instrument.
    std::vector<wmd_sub_instrument> sub_instruments;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_WMD_FILE_H
