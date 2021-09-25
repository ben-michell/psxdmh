// psxdmh/src/extract_audio.h
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

#ifndef PSXDMH_SRC_EXTRACT_AUDIO_H
#define PSXDMH_SRC_EXTRACT_AUDIO_H


namespace psxdmh
{


// Forwards.
class lcd_file;
class options;
class wmd_file;


// Extract a range of songs.
extern void extract_songs(const std::vector<uint16_t> &song_indexes, const wmd_file &wmd, const lcd_file &lcd, std::string output_name, const options &opts);

// Extract one track from a song.
extern void extract_track(uint16_t song_index, uint16_t track_index, const wmd_file &wmd, const lcd_file &lcd, std::string wav_file_name, const options &opts);

// Extract a range of patches from an LCD file.
extern void extract_patch(const std::vector<uint16_t> &patch_ids, const lcd_file &lcd, std::string output_name, const options &opts);


}; //namespace psxdmh


#endif // PSXDMH_SRC_EXTRACT_AUDIO_H

