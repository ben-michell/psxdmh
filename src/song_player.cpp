// psxdmh/src/song_player.cpp
// Playback manager for songs.
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
#include "options.h"
#include "song_player.h"
#include "wmd_file.h"


namespace psxdmh
{


//
// Construction.
//

song_player::song_player(size_t song_index, const wmd_file &wmd, const lcd_file &lcd, const options &opts)
{
    // Create the track players.
    assert(song_index < wmd.songs());
    assert(opts.sample_rate > 0);
    const wmd_song &song = wmd.song(song_index);
    for (size_t track_index = 0; track_index < song.tracks.size(); ++track_index)
    {
        m_tracks.push_back(std::unique_ptr<track_player>(new track_player(song_index, track_index, wmd, lcd, opts)));
    }
}


//
// Test whether the song playback is still running.
//

bool
song_player::is_running() const
{
    auto predicate = [](const std::unique_ptr<track_player> &track) { return track->is_running(); };
    return std::any_of(m_tracks.cbegin(), m_tracks.cend(), predicate);
}


//
// Get the next sample.
//

bool
song_player::next(stereo_t &stereo)
{
    // Accumulate samples from all tracks.
    stereo = 0.0;
    stereo_t temp;
    bool live = false;
    for (auto iter = m_tracks.cbegin(); iter != m_tracks.cend(); ++iter)
    {
        live = (*iter)->next(temp) || live;
        stereo += temp;
    }
    assert(live || !is_running());
    return live;
}


//
// Check if the song failed to repeat when a repeat was requested.
//

bool
song_player::failed_to_repeat() const
{
    auto predicate = [](const std::unique_ptr<track_player> &track) { return track->failed_to_repeat(); };
    return std::any_of(m_tracks.cbegin(), m_tracks.cend(), predicate);
}


}; //namespace psxdmh
