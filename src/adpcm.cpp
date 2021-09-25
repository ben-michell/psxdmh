// psxdmh/src/adpcm.cpp
// Decoding of ADPCM encoded audio data.
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

#include "global.h"

#include "adpcm.h"
#include "utility.h"


namespace psxdmh
{


// Tables used to decode ADPCM data.
const int32_t adpcm::m_pos_table[5] = { 0, 60, 115, 98, 122 };
const int32_t adpcm::m_neg_table[5] = { 0, 0, -52, -55, -60 };


//
// Construction.
//

adpcm::adpcm(const std::vector<uint8_t> &data, uint32_t play_count) :
    m_data(data), m_play_count(play_count),
    m_current(0), m_repeat(-1),
    m_s0(0), m_s1(0),
    m_buffer_next(PSXDMH_ADPCM_SAMPLES_PER_BLOCK)
{
    assert(data.size() > 0);
    assert(data.size() % PSXDMH_ADPCM_BLOCK_SIZE == 0);
    assert(is_final(data.data() + data.size() - PSXDMH_ADPCM_BLOCK_SIZE));
}


//
// Get the next sample.
//

bool
adpcm::next(mono_t &mono)
{
    // Return silence if the audio data has been exhausted.
    if (is_buffer_empty() && m_current < 0)
    {
        mono = 0.0;
        return false;
    }

    // Decode the next block when the buffer is empty.
    if (is_buffer_empty())
    {
        decode_block();
        next_block();
    }

    // Extract the next sample from the buffer.
    assert(!is_buffer_empty());
    mono = mono_t(m_buffer[m_buffer_next++]) / -SHRT_MIN;
    return true;
}


//
// Decode and buffer the current ADPCM encoded data block.
//

void
adpcm::decode_block()
{
    // Extract the unpacking control values.
    assert(m_current >= 0 && m_current < (int32_t) m_data.size());
    const uint8_t *block = m_data.data() + m_current;
    int32_t filter = block[0] >> 4;
    if (filter >= numberof(m_pos_table))
    {
        throw std::string("Corrupt ADPCM block (bad filter).");
    }
    int32_t shift = block[0] & 0x0f;

    // Remember repeat points.
    if (is_repeat_start(block))
    {
        m_repeat = m_current;
    }

    // Unpack the data nybbles into bytes for ease of processing.
    const uint8_t *data = block + 2;
    int8_t nybble[PSXDMH_ADPCM_SAMPLES_PER_BLOCK];
    for (auto unpack = 0; unpack < PSXDMH_ADPCM_SAMPLES_PER_BLOCK / 2; ++unpack)
    {
        nybble[2 * unpack] = int8_t(data[unpack] & 0x0f) << 4;
        nybble[2 * unpack + 1] = int8_t(data[unpack] & 0xf0);
    }

    // Decode the samples into the buffer.
    assert(is_buffer_empty());
    m_buffer_next = 0;
    for (auto decode = 0; decode < PSXDMH_ADPCM_SAMPLES_PER_BLOCK; ++decode)
    {
        // Calculate the next sample, and clip to prevent wrapping. The only
        // sound in Doom that gets clipped is patch 0x0b (the BFG explosion).
        int32_t s = (int32_t(nybble[decode]) << 8) >> shift;
        s += (m_s0 * m_pos_table[filter] + m_s1 * m_neg_table[filter] + 32) >> 6;
        m_buffer[decode] = int16_t(clamp(s, SHRT_MIN, SHRT_MAX));

        // Update the previous samples.
        m_s1 = m_s0;
        m_s0 = m_buffer[decode];
    }
}


//
// Move to the next block of data.
//

void
adpcm::next_block()
{
    // At the final block either stop or repeat.
    assert(m_current >= 0);
    const uint8_t *block = m_data.data() + m_current;
    if (is_final(block))
    {
        // Stop if the data doesn't repeat or only one repeat was requested.
        if (!is_repeat_jump(block) || m_repeat < 0 || m_play_count == 1)
        {
            m_current = -1;
        }
        // Adjust the play count (if finite) and repeat.
        else
        {
            m_play_count = decrement_play_count(m_play_count);
            m_current = m_repeat;
        }
    }
    // Otherwise move to the next block.
    else
    {
        m_current += PSXDMH_ADPCM_BLOCK_SIZE;
        assert(m_current <= (int32_t) m_data.size());
    }
}


//
// Edit a stream of ADPCM data.
//

void
adpcm::edit_adpcm(std::vector<uint8_t> &adpcm, size_t silence_start, size_t remove_end)
{
    // Zero blocks at the start, leaving their flags intact.
    assert(PSXDMH_ADPCM_BLOCK_SIZE * (silence_start + remove_end) <= adpcm.size());
    uint8_t *block = adpcm.data();
    while (silence_start-- > 0)
    {
        memset(block + 2, 0, PSXDMH_ADPCM_BLOCK_SIZE - 2);
        block += PSXDMH_ADPCM_BLOCK_SIZE;
    }

    // Remove blocks from the end, preserving the final / repeat flag.
    if (remove_end > 0)
    {
        block = adpcm.data() + adpcm.size() - PSXDMH_ADPCM_BLOCK_SIZE;
        uint8_t flags = block[1];
        block -= remove_end * PSXDMH_ADPCM_BLOCK_SIZE;
        block[1] = flags;
        adpcm.erase(adpcm.end() - remove_end * PSXDMH_ADPCM_BLOCK_SIZE, adpcm.end());
    }
}


//
// Find the offset of a repeat point within ADPCM data.
//

int32_t
adpcm::repeat_offset(const std::vector<uint8_t> &adpcm)
{
    // Two conditions must be met for a valid repeat: a repeat start flag must
    // be set on a block, and second the final block must have the repeat jump
    // flag set.
    assert(adpcm.size() > 0);
    assert(adpcm.size() % PSXDMH_ADPCM_BLOCK_SIZE == 0);
    int32_t block = (int32_t) adpcm.size() - PSXDMH_ADPCM_BLOCK_SIZE;
    assert(is_final(adpcm.data() + block));
    if (is_repeat_jump(adpcm.data() + block))
    {
        for (; block >= 0; block -= PSXDMH_ADPCM_BLOCK_SIZE)
        {
            if (is_repeat_start(adpcm.data() + block))
            {
                return block;
            }
        }
    }
    return -1;
}


}; //namespace psxdmh
