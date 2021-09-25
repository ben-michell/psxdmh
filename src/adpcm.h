// psxdmh/src/adpcm.h
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

#ifndef PSXDMH_SRC_ADPCM_H
#define PSXDMH_SRC_ADPCM_H


#include "module.h"


namespace psxdmh
{


// Number of bytes per ADPCM encoded data block.
#define PSXDMH_ADPCM_BLOCK_SIZE             (16)

// Number of audio samples generated from each data block.
#define PSXDMH_ADPCM_SAMPLES_PER_BLOCK      (28)


// Decoding of ADPCM encoded audio data. All errors are reported by a thrown
// std::string.
class adpcm : public module_mono
{
public:

    // Construction. The data vector contains ADPCM-encoded blocks of audio
    // data. Note that this class does not copy the data, instead relying on its
    // owner to keep the vector valid for the lifetime of this object. The
    // play_count controls how many times repeating sounds are played. A value
    // of 0 plays indefinitely, while any other value plays exactly that number
    // of times. This is ignored for non-repeating sounds.
    adpcm(const std::vector<uint8_t> &data, uint32_t play_count = 0);

    // Test whether the module is still generating output.
    virtual bool is_running() const { return !is_buffer_empty() || m_current >= 0; }

    // Get the next sample.
    virtual bool next(mono_t &mono);

    // Edit a stream of ADPCM data. Blocks at the start of the stream can be
    // silenced, and blocks at the end removed. Repeating patches are preserved.
    static void edit_adpcm(std::vector<uint8_t> &adpcm, size_t silence_start, size_t remove_end);

    // Check if an ADPCM data block is flagged as the start of a repeat.
    static bool is_repeat_start(const uint8_t *block) { assert(block != nullptr); return (block[1] & 0x04) == 0x04; }

    // Check if an ADPCM data block is flagged as the final block.
    static bool is_final(const uint8_t *block) { assert(block != nullptr); return (block[1] & 0x01) == 0x01; }

    // Check if an ADPCM data block is flagged as repeating after this block.
    static bool is_repeat_jump(const uint8_t *block) { assert(block != nullptr); return (block[1] & 0x03) == 0x03; }

    // Find the offset of a repeat point within ADPCM data. If no repeat is
    // found the return value will be negative.
    static int32_t repeat_offset(const std::vector<uint8_t> &adpcm);

private:

    // Test if the decoded audio buffer is empty.
    bool is_buffer_empty() const { return m_buffer_next >= PSXDMH_ADPCM_SAMPLES_PER_BLOCK; }

    // Decrement a play count. Infinite plays (a value of 0) don't change.
    static uint32_t decrement_play_count(uint32_t count) { return count > 0 ? count - 1: count; }

    // Decode and buffer the current ADPCM encoded data block.
    void decode_block();

    // Move to the next block of data. This handles repeating sounds.
    void next_block();

    // ADPCM-encoded audio data.
    const std::vector<uint8_t> &m_data;

    // Current position within the data. When this is negative it means that all
    // audio data has been exhausted.
    int32_t m_current;

    // Repeat point within the data. A negative value means no repeat. Zero or
    // positive is the offset within the data where to repeat.
    int32_t m_repeat;

    // Number of times to play repeating sounds. A value of 0 means repeat
    // indefinitely, while other values play exactly that many times.
    uint32_t m_play_count;

    // Previous two samples. The previous is s0, and the one before that in s1.
    int32_t m_s0;
    int32_t m_s1;

    // Buffered unpacked data block.
    int16_t m_buffer[PSXDMH_ADPCM_SAMPLES_PER_BLOCK];
    size_t m_buffer_next;

    // Tables used to decode ADPCM data.
    static const int32_t m_pos_table[5];
    static const int32_t m_neg_table[5];
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_ADPCM_H
