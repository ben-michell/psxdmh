// psxdmh/src/music_stream.cpp
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

#include "global.h"

#include "music_stream.h"


namespace psxdmh
{


//
// Construction.
//

music_stream::music_stream(const wmd_song_track &track, uint32_t ticks_per_minute) :
    m_track(track),
    m_position(0),
    m_caller_ticks_per_minute(ticks_per_minute),
    m_track_ticks_per_minute(uint32_t(track.ticks_per_beat) * uint32_t(track.beats_per_minute)),
    m_tick_position(0), m_tick_fraction(0),
    m_next_event_time(0)
{
    // Read the initial time delta.
    m_next_event_time = get_delta();
}


//
// Advance the music extraction by one caller tick.
//

void
music_stream::tick()
{
    // Increment the position.
    m_tick_fraction += m_track_ticks_per_minute;
    while (m_tick_fraction >= m_caller_ticks_per_minute)
    {
        m_tick_fraction -= m_caller_ticks_per_minute;
        m_tick_position++;
    }
}


//
// Attempt to extract an event from the stream for the current time.
//

bool
music_stream::get_event(music_event &ev)
{
    // Check if the stream is still running and the next event is due.
    if (have_event())
    {
        // Read the next event from the stream.
        uint8_t code = get_byte();
        ev.data_0 = 0;
        ev.data_1 = 0;
        switch (code)
        {
        // Note on.
        case 0x11:
            ev.code = music_event_code::note_on;
            ev.data_0 = get_byte();
            ev.data_1 = get_byte();
            break;

        // Note off.
        case 0x12:
            ev.code = music_event_code::note_off;
            ev.data_0 = get_byte();
            break;

        // Set instrument.
        case 0x07:
            ev.code = music_event_code::set_instrument;
            ev.data_0 = get_word();
            break;

        // Pitch bend.
        case 0x09:
            ev.code = music_event_code::pitch_bend;
            ev.data_0 = int16_t(get_word());
            break;

        // Set track master volume.
        case 0x0c:
            ev.code = music_event_code::volume;
            ev.data_0 = get_byte();
            break;

        // Set pan offset.
        case 0x0d:
            ev.code = music_event_code::pan_offset;
            ev.data_0 = get_byte();
            break;

        // Set marker point.
        case 0x23:
            ev.code = music_event_code::set_marker;
            ev.data_0 = (int32_t) (m_position - 1);
            break;

        // Jump to marker.
        case 0x20:
            ev.code = music_event_code::jump_to_marker;
            ev.data_0 = get_word();
            break;

        // Unknown code.
        case 0x0b:
            ev.code = music_event_code::unknown_0b;
            ev.data_0 = get_byte();
            break;

        // Unknown code.
        case 0x0e:
            ev.code = music_event_code::unknown_0e;
            ev.data_0 = get_byte();
            break;

        // End of stream.
        case 0x22:
            // Ensure the data pointer is at the end.
            ev.code = music_event_code::eos;
            m_position = m_track.data.size();
            break;

        // Abort if the code is unrecognised.
        default:
            throw std::string("Unsupported music stream event code $") + hex_byte(code) + ".";
        }

        // Read the next delta if not at the end of the stream.
        if (m_position < m_track.data.size())
        {
            m_next_event_time += get_delta();
        }

        // An event was extracted.
        return true;
    }

    // No event was extracted.
    return false;
}


//
// Set the current position in the stream.
//

void
music_stream::seek(size_t pos)
{
    if (pos > m_track.data.size())
    {
        throw std::string("Invalid seek position in music stream.");
    }
    m_position = pos;
}


//
// Extract the next byte from the music stream.
//

uint8_t
music_stream::get_byte()
{
    if ((m_position + 1) > m_track.data.size())
    {
        throw std::string("Corrupt music data: attempt to read beyond the end of the stream.");
    }
    return m_track.data[m_position++];
}


//
// Extract the next two bytes from the music stream as a 16-bit value in
// little-endian order.
//

uint16_t
music_stream::get_word()
{
    if ((m_position + 2) > m_track.data.size())
    {
        throw std::string("Corrupt music data: attempt to read beyond the end of the stream.");
    }
    uint16_t temp = uint16_t(m_track.data[m_position]) | (uint16_t(m_track.data[m_position + 1]) << 8);
    m_position += 2;
    return temp;
}


//
// Read a variable length time delta from the music stream.
//

uint32_t
music_stream::get_delta()
{
    // The time delta is accumulated from the low 7 bits of bytes. The top bit
    // is set when there is another byte to follow.
    uint32_t delta = 0;
    uint8_t byte;
    do
    {
        byte = get_byte();
        delta = (delta << 7) | uint32_t(byte & 0x7f);
    }
    while ((byte & 0x80) != 0);
    return delta;
}


}; //namespace psxdmh
