// psxdmh/src/music_stream.h
// Parsing of MIDI-style music events from WMD song tracks.
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

#ifndef PSXDMH_SRC_MUSIC_STREAM_H
#define PSXDMH_SRC_MUSIC_STREAM_H


#include "wmd_file.h"


namespace psxdmh
{


// Music stream event codes.
enum class music_event_code
{
    // Note on: data_0 contains the note number (0x00 to 0x7f) and data_1
    // contains the note volume (0x00 to 0x7f).
    note_on,

    // Note off: data_0 contains the note number (0x00 to 0x7f).
    note_off,

    // Set instrument: data_0 contains the instrument number. This is also
    // specified in the track header, and can therefore be ignored.
    set_instrument,

    // Pitch bend: data_0 contains the bend (-0x2000 to 0x2000). This affects
    // all notes currently playing in the track.
    pitch_bend,

    // Set track master volume: data_0 contains the volume (0x00 to 0x7f).
    volume,

    // Set pan offset: data0 contains the pan value (0x00 to 0x7f). This offset
    // is applied to all notes playing in the track.
    pan_offset,

    // Set marker point: data_0 contains the stream offset of the marker code.
    set_marker,

    // Jump to marker: data_0 contains the marker number.
    jump_to_marker,

    // Unknown 0x0b code: data_0 contains an 8-bit value. Used only once in song
    // 111. I haven't been able to hear any effect from this code.
    unknown_0b,

    // Unknown 0x0e code: data_0 contains an 8-bit value. Used only in songs 90,
    // 92, 110, 111, and 112. Always appears in pairs, first with a value of
    // 0x7f, followed (usually quickly) by another with a value of 0x00. I
    // haven't been able to hear any effect from this code.
    unknown_0e,

    // End of stream.
    eos
};


// Music stream event.
struct music_event
{
    // Event code.
    music_event_code code;

    // Event code parameters. The meaning of these values depends on the event
    // code. Refer to the event code definitions in music_event_code.
    int32_t data_0;
    int32_t data_1;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// WMD music stream parser. All errors are reported by a thrown std::string.
class music_stream
{
public:

    // Construction. The caller must ensure the track object remains valid for
    // the lifetime of this music stream. The tick rate is the number of caller
    // ticks per minute. This is converted into the track tick rate by the
    // extraction function. Note that the constructor will throw an exception if
    // it encounters an error.
    music_stream(const wmd_song_track &track, uint32_t ticks_per_minute);

    // Test whether there are more events in the music stream. If the stream has
    // reached the end (an eos event has been extracted) this method will return
    // false.
    bool is_running() const { return m_position < m_track.data.size(); }

    // Advance the music extraction by one caller tick.
    void tick();

    // Test if one or more events are available for extraction.
    bool have_event() const { return m_position < m_track.data.size() && m_next_event_time <= m_tick_position; }

    // Attempt to extract an event from the stream for the current time. The
    // return value is true if an event was extracted and false if not. Since
    // more than one event can occur at the same time this method should be
    // called repeatedly until it returns false.
    bool get_event(music_event &ev);

    // Set the current position in the stream. This can be used to handle
    // repeating music data.
    void seek(size_t pos);

private:

    // Extract the next byte from the music stream. An attempt to read beyond
    // the end of the stream is treated as a fatal error.
    uint8_t get_byte();

    // Extract the next two bytes from the music stream as a 16-bit value in
    // little-endian order. An attempt to read beyond the end of the stream is
    // treated as a fatal error.
    uint16_t get_word();

    // Read a variable length time delta from the music stream.
    uint32_t get_delta();

    // Track containing the music stream.
    const wmd_song_track &m_track;

    // Current position within the track music stream.
    size_t m_position;

    // Number of caller ticks per minute.
    uint32_t m_caller_ticks_per_minute;

    // Number of track ticks per minute.
    uint32_t m_track_ticks_per_minute;

    // Current position within the track expressed in track ticks. The whole
    // number of ticks is in m_tick_position, and the fractional position is in
    // m_tick_fraction. There are m_caller_ticks_per_minute fractional positions
    // per whole position, with m_track_ticks_per_minute added for each caller
    // tick.
    uint32_t m_tick_position;
    uint32_t m_tick_fraction;

    // Track time for the next event in the stream.
    uint32_t m_next_event_time;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_MUSIC_STREAM_H
