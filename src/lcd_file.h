// psxdmh/src/lcd_file.h
// Parsing of LCD format files.
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

#ifndef PSXDMH_SRC_LCD_FILE_H
#define PSXDMH_SRC_LCD_FILE_H


#include "adpcm.h"
#include "utility.h"


namespace psxdmh
{


// Sampling rate for all patches.
#define PSXDMH_PATCH_FREQUENCY      (11025)


// Details of a patch in an LCD file.
struct patch
{
    // Construction.
    patch() {}
    patch(uint16_t new_id, const std::vector<uint8_t> &new_adpcm) : id(new_id), adpcm(new_adpcm) {}

    // Patch ID.
    uint16_t id;

    // ADPCM encoded audio data.
    std::vector<uint8_t> adpcm;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// LCD file parser. All errors are reported by a thrown std::string.
class lcd_file
{
public:

    // Construction. Note that the constructor will throw an exception if it
    // encounters an error.
    lcd_file();
    lcd_file(std::string file_name);

    // Test if the file is empty.
    bool is_empty() const { return m_patches.empty(); }

    // Maximum patch ID in the LCD file.
    uint16_t maximum_patch_id() const;

    // Get a patch by ID. Returns nullptr if not found.
    const patch *patch_by_id(uint16_t id) const;

    // Set a patch by ID. If a patch with the same ID already exists it will be
    // overwritten, otherwise the patch will be appended to the end of the
    // collection.
    void set_patch_by_id(uint16_t id, const std::vector<uint8_t> &adpcm);

    // Load from a file. The current contents of this object are overwritten.
    void parse(std::string file_name);

    // Store the contents of this object in a file.
    void write(std::string file_name) const;

    // Merge another file with this one. Any patches in the other file which
    // aren't in this one are copied.
    void merge(const lcd_file &lcd);

    // Sort the patches into ID order.
    void sort();

    // Apply repairs to patches with clicks and pops.
    void repair_patches();

    // Dump details about the LCD file.
    void dump() const;

private:

    // Details of fixes to apply to patches.
    struct patch_fix
    {
       // Patch ID.
       uint16_t id;

       // Details used to validate the patch.
       size_t size;
       int32_t repeat_offset;

       // Number of blocks to silence at the start, and the number to remove
       // from the end.
       size_t silence_start_blocks;
       size_t remove_end_blocks;
    };

    // Convert from a byte count to the number of ADPCM blocks.
    static uint32_t bytes_to_blocks(uint32_t bytes) { return bytes / PSXDMH_ADPCM_BLOCK_SIZE; }

    // Convert from ADPCM blocks to their playback time.
    static double blocks_to_seconds(uint32_t blocks) { return double(blocks) * PSXDMH_ADPCM_SAMPLES_PER_BLOCK / PSXDMH_PATCH_FREQUENCY; }

    // Default capacity to use for the patches vector. This is large enough that
    // the vector should never need to resize itself.
    static const size_t m_default_capacity;

    // Fixes for patches.
    static const patch_fix m_patch_fixes[];

    // Collection of patches in the LCD file.
    std::vector<patch> m_patches;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_LCD_FILE_H
