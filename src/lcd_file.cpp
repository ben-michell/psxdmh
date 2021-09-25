// psxdmh/src/lcd_file.cpp
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

#include "global.h"

#include "lcd_file.h"
#include "safe_file.h"


namespace psxdmh
{


// Default capacity to use for the patches vector.
const size_t lcd_file::m_default_capacity = 160;


// Fixes for patches.
const lcd_file::patch_fix lcd_file::m_patch_fixes[] =
{
    {  96,   45744,    16,   2,  1 }, // Song 94.
    { 102,   86016, 45248,   2,  0 }, // Song 97.
    { 116,   81520,     0,   0, 16 }, // Song 106.
    { 130,   44928,    16,   0,  2 }, // Song 114.
};


//
// Construction.
//

lcd_file::lcd_file()
{
    m_patches.reserve(m_default_capacity);
}


//
// Construction.
//

lcd_file::lcd_file(std::string file_name)
{
    parse(file_name);
}


//
// Maximum patch ID in the LCD file.
//

uint16_t
lcd_file::maximum_patch_id() const
{
    uint16_t maximum = 0;
    std::for_each(m_patches.cbegin(), m_patches.cend(), [&maximum](const struct patch &p) { maximum = std::max(maximum, p.id); });
    return maximum;
}


//
// Get a patch by ID.
//

const patch *
lcd_file::patch_by_id(uint16_t id) const
{
    auto predicate = [id](const struct patch &p) { return p.id == id; };
    auto patch = std::find_if(m_patches.cbegin(), m_patches.cend(), predicate);
    return patch != m_patches.cend() ? &*patch : nullptr;
}


//
// Set a patch by ID.
//

void
lcd_file::set_patch_by_id(uint16_t id, const std::vector<uint8_t> &adpcm)
{
    // Attempt to update an existing patch.
    assert(adpcm.size() > 0);
    assert(adpcm.size() % PSXDMH_ADPCM_BLOCK_SIZE == 0);
    auto p = std::find_if(m_patches.begin(), m_patches.end(), [id](struct patch &patch) { return patch.id == id; });
    if (p != m_patches.end())
    {
        p->adpcm = adpcm;
    }
    // Append a new patch.
    else
    {
        m_patches.push_back(patch(id, adpcm));
    }
}


//
// Load from a file.
//

void
lcd_file::parse(std::string file_name)
{
    // Read the header: the number of patches and their IDs.
    safe_file file(file_name, file_mode::read);
    m_patches.clear();
    m_patches.reserve(m_default_capacity);
    m_patches.resize(file.read_16_le());
    std::vector<patch>::iterator iter;
    for (iter = m_patches.begin(); iter != m_patches.end(); ++iter)
    {
        iter->id = file.read_16_le();
    }

    // Locate the data for each patch in the LCD file. Patches start at offset
    // 0x800 (the size of 1 block on the CD).
    file.seek(0x800);
    uint8_t block[PSXDMH_ADPCM_BLOCK_SIZE];
    for (iter = m_patches.begin(); iter != m_patches.end(); ++iter)
    {
        // Skip the header (a block of 16 zero bytes).
        static const uint8_t sixteen_zeros[16] = { 0 };
        file.read(block, sizeof(block));
        if (memcmp(block, sixteen_zeros, sizeof(block)) != 0)
        {
            throw std::string("Invalid patch header in '") + file_name + "'.";
        }

        // Identify the patches. The ideal way to do this would be to use the
        // patch sizes from the WMD file, but the algorithm used here works for
        // all LCD files in Doom and Final Doom, and it means we can load an LCD
        // file without needing the WMD.
        while (!file.eof())
        {
            // Store the next block of ADPCM data.
            file.read(block, sizeof(block));
            iter->adpcm.insert(iter->adpcm.end(), block, block + numberof(block));

            // Stop when an end point is found.
            if (adpcm::is_final(block))
            {
                break;
            }
        }

        // Skip any padding before the next patch, which can be identified by
        // the header of 16 zeros.
        while (!file.eof())
        {
            size_t pos = file.tell();
            file.read(block, sizeof(block));
            if (memcmp(block, sixteen_zeros, sizeof(block)) == 0)
            {
                file.seek(pos);
                break;
            }
        }
    }
}


//
// Store the contents of this object in a file.
//

void
lcd_file::write(std::string file_name) const
{
    assert(!is_empty());
    safe_file lcd_file(file_name, file_mode::write);
    lcd_file.write_16_le((uint16_t) m_patches.size());
    std::vector<patch>::const_iterator patch;
    for (patch = m_patches.cbegin(); patch != m_patches.cend(); ++patch)
    {
        lcd_file.write_16_le(patch->id);
    }
    lcd_file.write_zeros(0x800 - lcd_file.tell());
    for (patch = m_patches.begin(); patch != m_patches.end(); ++patch)
    {
        lcd_file.write_zeros(16);
        lcd_file.write(patch->adpcm.data(), patch->adpcm.size());
    }
    lcd_file.close();
}


//
// Merge another file with this one.
//

void
lcd_file::merge(const lcd_file &lcd)
{
    assert(this != &lcd);
    for (auto iter = lcd.m_patches.cbegin(); iter != lcd.m_patches.cend(); ++iter)
    {
        if (patch_by_id(iter->id) == nullptr)
        {
            m_patches.push_back(patch(iter->id, iter->adpcm));
        }
    }
}


//
// Sort the patches into ID order.
//

void
lcd_file::sort()
{
    auto compare = [](const patch &p0, const patch &p1) { return p0.id < p1.id; };
    std::sort(m_patches.begin(), m_patches.end(), compare);
}


//
// Apply repairs to patches with clicks and pops.
//

void
lcd_file::repair_patches()
{
    for (auto fix = 0; fix < numberof(m_patch_fixes); ++fix)
    {
        const patch *patch = patch_by_id(m_patch_fixes[fix].id);
        if (patch != nullptr)
        {
            // Ensure the details of the patch correspond to the fix.
            int32_t repeat = adpcm::repeat_offset(patch->adpcm);
            if (m_patch_fixes[fix].size != patch->adpcm.size()
                || (repeat >= 0 && repeat != m_patch_fixes[fix].repeat_offset)
                || (repeat < 0 && m_patch_fixes[fix].repeat_offset < 0))
            {
                throw std::string("Patch ") + int_to_string(patch->id) + " can't be fixed: the details of the patch don't match the expected values.";
            }

            // Edit the ADPCM data and update the LCD.
            auto edit = patch->adpcm;
            adpcm::edit_adpcm(edit, m_patch_fixes[fix].silence_start_blocks, m_patch_fixes[fix].remove_end_blocks);
            set_patch_by_id(patch->id, edit);
        }
    }
}


//
// Dump details about the LCD file.
//

void
lcd_file::dump() const
{
    // Dump the contents of the LCD file.
    for (size_t index = 0; index < m_patches.size(); ++index)
    {
        const patch &patch = m_patches[index];
        printf("Patch %d:\n", (int) index);
        printf("  ID: %u ($%02x)\n", patch.id, patch.id);
        uint32_t bytes = (uint32_t) patch.adpcm.size();
        uint32_t blocks = bytes_to_blocks(bytes);
        printf("  Length: %u bytes, %u blocks, %.3lf seconds\n", bytes, blocks, blocks_to_seconds(blocks));
        int32_t repeat = adpcm::repeat_offset(patch.adpcm);
        if (repeat >= 0)
        {
            blocks = bytes_to_blocks(repeat);
            printf("  Non-repeated start: %u bytes, %u blocks, %.3lf seconds\n", repeat, blocks, blocks_to_seconds(blocks));
            bytes = (uint32_t) (patch.adpcm.size() - repeat);
            blocks = bytes_to_blocks(bytes);
            printf("  Repeat length: %u bytes, %u blocks, %.3lf seconds\n", bytes, blocks, blocks_to_seconds(blocks));
        }
        else
        {
            printf("  No repeat.\n");
        }
        printf("\n");
    }
}


}; //namespace psxdmh
