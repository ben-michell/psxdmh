// psxdmh/src/reverb.h
// PSX SPU reverb effect emulation.
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

#ifndef PSXDMH_SRC_REVERB_H
#define PSXDMH_SRC_REVERB_H


#include "module.h"
#include "resampler.h"
#include "splitter.h"
#include "utility.h"


namespace psxdmh
{


// Reverb presets.
enum reverb_preset
{
    rp_off,
    rp_room,
    rp_studio_small,
    rp_studio_medium,
    rp_studio_large,
    rp_hall,
    rp_half_echo,
    rp_space_echo,

    rp_number_of_presets,

    rp_auto = rp_number_of_presets
};


// Get the name of a reverb preset.
extern std::string reverb_to_string(reverb_preset preset);


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Wrapper for the emulation of the PSX SPU reverb effect. This takes care of
// splitting off an audio stream for reverb, resampling to 22.05 kHz, generating
// the reverb, resampling back to the original rate, and mixing with the
// original audio.
class reverb : public module_stereo
{
public:

    // Construction.
    reverb(module_stereo *source, uint32_t sample_rate, reverb_preset preset, stereo_t volume, uint32_t sinc_window);

    // Test whether the module is still generating output.
    virtual bool is_running() const;

    // Get the next sample.
    virtual bool next(stereo_t &s);

private:

    // Original and reverb effect streams. These are mixed by this module.
    std::unique_ptr<splitter_stereo> m_original_stream;
    std::unique_ptr<module_stereo> m_reverb_stream;
};


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// PSX SPU reverb effect emulation.
class reverb_core : public module_stereo
{
public:

    // Construction.
    reverb_core(module_stereo *source, reverb_preset preset, stereo_t volume);

    // Test whether the module is still generating output.
    virtual bool is_running() const;

    // Get the next sample.
    virtual bool next(stereo_t &s);

private:

    // Read a value from the work area. The offset is wrapped into the range
    // used by the buffer.
    mono_t read_buffer(size_t offset) const { return m_buffer[wrap_offset(m_current + offset)]; }

    // Write a value into the work area.
    void write_buffer(size_t offset, mono_t v) { m_buffer[wrap_offset(m_current + offset)] = flush_denorm(v); }

    // Wrap an offset into the range used by the buffer. Since a simple (and
    // fast) calculation is used, the assert checks that the offset is
    // relatively close to the valid range.
    size_t wrap_offset(size_t offset) const { assert(offset < 2 * m_buffer.size()); return offset < m_buffer.size() ? offset : offset - m_buffer.size(); }

    // Convert a SPU register value into a volume as a mono_t.
    static mono_t reg_to_volume(uint16_t v) { return mono_t(int16_t(v)) / -SHRT_MIN; }

    // Convert a SPU register value from bytes/8 offset to an array offset.
    static size_t reg_to_offset(uint16_t v) { assert(v <= 0x7fff); return size_t(v) * 8U / sizeof(int16_t); }

    // Reverb configuration.
    reverb_preset m_preset;

    // Reverb output volume.
    stereo_t m_volume;

    // Work area.
    std::vector<mono_t> m_buffer;

    // Current position within the buffer.
    size_t m_current;

    // SPU reverb registers. Volume-related registers are stored as mono_t,
    // while address/offset registers (specified as bytes/8) are converted to
    // array offsets.
    size_t m_dapf1;   // All pass filter offset 1.
    size_t m_dapf2;   // All pass filter offset 1.
    mono_t m_viir;    // Reflection volume 1.
    mono_t m_vcomb1;  // Comb volume 1.
    mono_t m_vcomb2;  // Comb volume 2.
    mono_t m_vcomb3;  // Comb volume 3.
    mono_t m_vcomb4;  // Comb volume 4.
    mono_t m_vwall;   // Reflection volume 2.
    mono_t m_vapf1;   // All pass filter volume 1.
    mono_t m_vapf2;   // All pass filter volume 2.
    size_t m_mlsame;  // Same side reflection address 1 left.
    size_t m_mrsame;  // Same side reflection address 1 right.
    size_t m_mlcomb1; // Comb address 1 left.
    size_t m_mrcomb1; // Comb address 1 right.
    size_t m_mlcomb2; // Comb address 2 left.
    size_t m_mrcomb2; // Comb address 2 right.
    size_t m_dlsame;  // Same side reflection address 2 left.
    size_t m_drsame;  // Same side reflection address 2 right.
    size_t m_mldiff;  // Different side reflect address 1 left.
    size_t m_mrdiff;  // Different side reflect address 1 right.
    size_t m_mlcomb3; // Comb address 3 left.
    size_t m_mrcomb3; // Comb address 3 right.
    size_t m_mlcomb4; // Comb address 4 left.
    size_t m_mrcomb4; // Comb address 4 right.
    size_t m_dldiff;  // Different side reflect address 2 left.
    size_t m_drdiff;  // Different side reflect address 2 right.
    size_t m_mlapf1;  // All pass filter address 1 left.
    size_t m_mrapf1;  // All pass filter address 1 right.
    size_t m_mlapf2;  // All pass filter address 2 left.
    size_t m_mrapf2;  // All pass filter address 2 right.
    mono_t m_vlin;    // Input volume left.
    mono_t m_vrin;    // Input volume right.

    // Address offsets derived from the registers.
    size_t m_mlsame_1;     // m_mlsame - 1
    size_t m_mrsame_1;     // m_mrsame - 1
    size_t m_mldiff_1;     // m_mldiff - 1
    size_t m_mrdiff_1;     // m_mrdiff - 1
    size_t m_mlapf1_dapf1; // m_mlapf1 - m_dapf1
    size_t m_mrapf1_dapf1; // m_mrapf1 - m_dapf1
    size_t m_mlapf2_dapf2; // m_mlapf2 - m_dapf2
    size_t m_mrapf2_dapf2; // m_mrapf2 - m_dapf2

    // Magnitude representing the threshold of silence at the reverb unit's
    // volume.
    mono_t m_silence;

    // Flag when the buffer is known to be fully silent. This prevents calls to
    // is_running() from taking an eternity once reverb has stopped.
    mutable bool m_buffer_is_silent;

    // Location of the last non-silent sample found in the buffer.
    mutable size_t m_last_unsilent_sample;

    // Registers for each preset.
    static const uint16_t m_registers[rp_number_of_presets][32];

    // Buffer size for each preset.
    static const size_t m_buffer_size[rp_number_of_presets];
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_REVERB_H
