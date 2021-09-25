// psxdmh/src/song_player.h
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

#ifndef PSXDMH_SRC_SONG_PLAYER_H
#define PSXDMH_SRC_SONG_PLAYER_H


#include "module.h"
#include "track_player.h"


namespace psxdmh
{


// Forwards.
class lcd_file;
class options;
class wmd_file;


// Playback manager for all tracks in a song.
class song_player : public module_stereo
{
public:

    // Construction. The caller must ensure that the WMD file and LCD set remain
    // valid for the life of this object.
    song_player(size_t song_index, const wmd_file &wmd, const lcd_file &lcd, const options &opts);

    // Test whether the song playback is still running.
    virtual bool is_running() const;

    // Get the next sample. The returned sample is the unscaled result of adding
    // together all currently playing notes from all tracks.
    virtual bool next(stereo_t &stereo);

    // Check if the song failed to repeat when a repeat was requested.
    bool failed_to_repeat() const;

private:

    // Players for each track.
    std::vector<std::unique_ptr<track_player>> m_tracks;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_SONG_PLAYER_H
