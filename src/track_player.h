// psxdmh/src/track_player.h
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

#ifndef PSXDMH_SRC_TRACK_PLAYER_H
#define PSXDMH_SRC_TRACK_PLAYER_H


#include "channel.h"
#include "module.h"
#include "music_stream.h"
#include "options.h"


namespace psxdmh
{


// Forwards.
class lcd_file;
class wmd_file;


// Playback manager for a single track.
class track_player : public module_stereo
{
public:

    // Construction. The caller must ensure that the WMD file and LCD set remain
    // valid for the life of this object.
    track_player(size_t song_index, size_t track_index, const wmd_file &wmd, const lcd_file &lcd, const options &opts);

    // Test whether the track playback is still running. This includes any
    // channels which haven't finished playing a note yet, even if all music
    // data for the track has been exhausted.
    virtual bool is_running() const;

    // Get the next sample. The returned sample is the unscaled result of adding
    // together all currently playing notes for this track.
    virtual bool next(stereo_t &stereo);

    // Check if the track failed to repeat when a repeat was requested.
    bool failed_to_repeat() const { return m_play_count > 1; }

private:

    // Create a new channel to play a note. Valid notes are 0x00 to 0x7f, and
    // valid volumes are 0x00 to 0x7f.
    void start_note(uint8_t note, uint8_t volume);

    // Adjust a pan value to account for stereo width expansion.
    uint8_t adjust_stereo_effect(uint8_t pan) const;

    // WMD file supplying music data.
    const wmd_file &m_wmd;

    // LCD file supplying patches.
    const lcd_file &m_lcd;

    // Output sample rate.
    const uint32_t m_sample_rate;

    // Resampling configuration.
    const uint32_t m_sinc_window;

    // Whether to enforce the maximum playback frequency limit of a real PSX.
    const bool m_limit_frequency;

    // Whether to repair patches.
    const bool m_repair_patches;

    // Number of remaining times to play the track. A value of 0 means repeat
    // indefinitely, while other values play exactly that many times.
    uint32_t m_play_count;

    // Instrument used by this track.
    size_t m_instrument_index;

    // Details of repeating song data.
    bool m_repeat;
    size_t m_repeat_start;

    // Music stream parser.
    music_stream m_stream;

    // Master track volume.
    mono_t m_track_volume;

    // Pan offset for the track. This offset is applied to all notes that play
    // in this track.
    int m_pan_offset;

    // Stereo width adjustment.
    mono_t m_stereo_width;

    // Current pitch bend at a sensitivity of 1.
    mono_t m_unit_pitch_bend;

    // Active channels.
    std::vector<std::unique_ptr<channel>> m_channels;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_TRACK_PLAYER_H
