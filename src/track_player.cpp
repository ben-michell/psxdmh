// psxdmh/src/track_player.cpp
// Playback manager for a single track.
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

#include "lcd_file.h"
#include "track_player.h"
#include "wmd_file.h"


namespace psxdmh
{


//
// Construction.
//

track_player::track_player(size_t song_index, size_t track_index, const wmd_file &wmd, const lcd_file &lcd, const options &opts) :
    m_wmd(wmd), m_lcd(lcd),
    m_sample_rate(opts.sample_rate),
    m_sinc_window(opts.sinc_window),
    m_limit_frequency(!opts.unlimited_frequency),
    m_repair_patches(opts.repair_patches),
    m_play_count(opts.play_count),
    m_stream(wmd.track(song_index, track_index), opts.sample_rate * 60),
    m_track_volume(1.0),
    m_pan_offset(0), m_stereo_width(opts.stereo_width),
    m_unit_pitch_bend(0.0)
{
    // Get the instrument and repeat details from the track.
    assert(opts.sample_rate > 0);
    assert(m_stereo_width >= -1.0 && m_stereo_width <= 1.0);
    const wmd_song_track &track = wmd.track(song_index, track_index);
    m_instrument_index = track.instrument;
    m_repeat = track.repeat;
    m_repeat_start = track.repeat_start;
}


//
// Test whether the track playback is still running.
//

bool
track_player::is_running() const
{
    // The track is running if the stream is still running, channels are still
    // playing, or the track will repeat.
    return !m_channels.empty()
        || (m_repeat && (m_play_count == 0 || m_play_count > 1))
        || m_stream.is_running();
}


//
// Get the next sample.
//

bool
track_player::next(stereo_t &stereo)
{
    // Process all current events.
    music_event ev;
    size_t index;
    bool live = !m_channels.empty() || m_stream.is_running();
    while (m_stream.get_event(ev))
    {
        live = true;
        switch (ev.code)
        {
        case music_event_code::note_on:
            // Validate the parameters.
            if (ev.data_0 < 0 || ev.data_0 > 0x7f)
            {
                throw std::string("Invalid note number in note on event.");
            }
            if (ev.data_1 < 0 || ev.data_1 > 0x7f)
            {
                throw std::string("Invalid volume in note on event.");
            }

            // Start a channel playing the note.
            start_note(ev.data_0, ev.data_1);
            break;

        case music_event_code::note_off:
            // Validate the parameter.
            if (ev.data_0 < 0 || ev.data_0 > 0x7f)
            {
                throw std::string("Invalid note number in note off event.");
            }

            // Look for channels playing the note and release them. There may be
            // more than one instance of a given note playing simultaneously due
            // to notes not disappearing immediately after being released.
            for (index = 0; index < m_channels.size(); ++index)
            {
                if (m_channels[index]->user_data() == (uint32_t) ev.data_0)
                {
                    m_channels[index]->release();
                }
            }
            break;

        case music_event_code::set_instrument:
            // This is ignored as the instrument never changes, and is already
            // set from the track header.
            break;

        case music_event_code::pitch_bend:
            // Validate the parameter.
            if (ev.data_0 < -0x2000 || ev.data_0 > 0x2000)
            {
                throw std::string("Invalid bend in pitch bend event.");
            }

            // Calculate the new unit pitch bend.
            m_unit_pitch_bend = mono_t(ev.data_0) / 0x2000 / 12;

            // Apply the pitch bend to every active channel.
            for (index = 0; index < m_channels.size(); ++index)
            {
                m_channels[index]->frequency(m_wmd.note_to_frequency(m_instrument_index, m_channels[index]->user_data(), m_unit_pitch_bend));
            }
            break;

        case music_event_code::volume:
            // Validate the volume.
            if (ev.data_0 < 0x00 || ev.data_0 > 0x7f)
            {
                throw std::string("Invalid volume in track volume event.");
            }

            // Convert the volume into floating point form and remember it for
            // future notes. The volume should probably be applied to all active
            // channels, but as this always appears before any notes it doesn't
            // really matter.
            m_track_volume = mono_t(ev.data_0) / 0x7f;
            break;

        case music_event_code::pan_offset:
            // Validate the pan.
            if (ev.data_0 < 0x00 || ev.data_0 > 0x7f)
            {
                throw std::string("Invalid pan in track pan event.");
            }

            // Convert the pan into a zero-based offset and remember it for
            // future notes. As with the volume above, this should probably be
            // applied to all active channels, but it likewise always appears
            // before any notes and so it doesn't matter.
            m_pan_offset = int(ev.data_0) - 0x40;
            break;

        case music_event_code::set_marker:
            // This is ignored as the repeat point is available from the track
            // header.
            break;

        case music_event_code::jump_to_marker:
            // Test if the caller wants this repeat, which is the case unless
            // the play count has reached 1.
            if (m_play_count != 1)
            {
                // Decrement the plays left for finite repeats.
                if (m_play_count > 0)
                {
                    m_play_count--;
                }

                // Jump to the repeat position.
                if (m_repeat)
                {
                    m_stream.seek(m_repeat_start);
                }
            }
            break;

        case music_event_code::unknown_0b:
        case music_event_code::unknown_0e:
            // Ignore unknown events.
            break;

        case music_event_code::eos:
            // There's no need to handle the end of stream here as it's tested
            // explicitly elsewhere.
            break;

        default:
            assert(!"Unhandled music stream event.");
            break;
        }
    }

    // Advance the music stream by one tick.
    if (m_stream.is_running())
    {
        m_stream.tick();
    }

    // Accumulate from all active channels, removing finished channels.
    stereo = 0.0;
    stereo_t temp;
    for (index = 0; index < m_channels.size();)
    {
        if (m_channels[index]->next(temp))
        {
            stereo += temp;
            index++;
        }
        else
        {
            m_channels.erase(m_channels.begin()+index);
        }
    }
    assert(live || !is_running());
    return live;
}


//
// Create a new channel to play a note.
//

void
track_player::start_note(uint8_t note, uint8_t volume)
{
    // Get the sub-instrument.
    assert(note <= 0x7f);
    assert(volume <= 0x7f);
    const wmd_sub_instrument &sub_instrument = m_wmd.instrument(m_instrument_index).sub_instrument(note);

    // Combine the master track, sub-instrument and note volumes.
    mono_t combined_volume = m_track_volume * mono_t(sub_instrument.volume) / 0x7f * mono_t(volume) / 0x7f;

    // Find the patch.
    const patch *patch = m_lcd.patch_by_id(sub_instrument.patch);
    if (patch == nullptr)
    {
        throw std::string("Unable to locate patch with id ") + int_to_string(sub_instrument.patch) + " in any LCD file.";
    }

    // Map the note to a frequency.
    uint32_t frequency = m_wmd.note_to_frequency(m_instrument_index, note, m_unit_pitch_bend);

    // Start the note playing. Store the note number as the channel user data.
    uint8_t pan = (uint8_t) clamp(int(sub_instrument.pan) + m_pan_offset, 0x00, 0x7f);
    pan = adjust_stereo_effect(pan);
    channel *c = new channel(patch, frequency, combined_volume, pan, sub_instrument.spu_ads, sub_instrument.spu_sr, m_sample_rate, m_sinc_window, m_limit_frequency, m_repair_patches);
    c->user_data(note);
    m_channels.push_back(std::unique_ptr<channel>(c));
}


//
// Adjust a pan value to account for stereo width expansion.
//

uint8_t
track_player::adjust_stereo_effect(uint8_t pan) const
{
    assert(pan <= 0x7f);
    if (m_stereo_width != 0.0)
    {
        // Remap the pan from [0x00, 0x7f] to [-1.0, 1.0]. Although the halfway
        // point in this range is between 0x3f and 0x40 (63.5), the songs use a
        // value of 0x40 (64) to represent centre. To preserve this convention
        // the remapping below is tweaked accordingly.
        const mono_t centre = 64.0;
        const mono_t left_range = centre - 0;
        const mono_t right_range = 127 - centre;
        mono_t remap = (pan - centre) / (pan < centre ? left_range : right_range);
        assert(remap >= -1.0 && remap <= 1.0);

        // Adjust the pan wider or narrower. A negative adjustment reduces the
        // stereo effect with -1.0 producing near-mono (apart from any sounds at
        // the far left or far right). Zero adjustment has no effect. A positive
        // adjustment increases the stereo effect, with 1.0 pushing anything not
        // at the centre significantly to the left or right.
        const mono_t strength = (mono_t) pow(4, -m_stereo_width);
        assert(strength >= 0.25 && strength <= 4.0);
        remap = (remap >= 0 ? 1 : -1) * pow(::fabs(remap), strength);
        if (isnan(remap))
        {
            remap = 0.0;
        }
        assert(remap >= -1.0 && remap <= 1.0);

        // Remap back to the [0x0, 0x7f] range, again preserving 0x40 as centre.
        int new_pan = (int) floor(remap * (remap < 0.0 ? left_range : right_range) + centre + 0.5);
        pan = (uint8_t) clamp(new_pan, 0x00, 0x7f);
    }
    return pan;
}


}; //namespace psxdmh
