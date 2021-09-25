// psxdmh/src/reverb.cpp
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

#include "global.h"

#include "filter.h"
#include "reverb.h"


namespace psxdmh
{


// Sample rate in the reverb effect core.
#define PSXDMH_REVERB_RATE      (22050)


//
// Get the name of a reverb preset.
//

std::string
reverb_to_string(reverb_preset preset)
{
    assert(preset >= 0 && preset < rp_number_of_presets);
    static const std::string names[rp_number_of_presets] =
    {
        "off",
        "room",
        "studio-small",
        "studio-medium",
        "studio-large",
        "hall",
        "half-echo",
        "space-echo"
    };
    return names[preset];
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Construction.
//

reverb::reverb(module_stereo *source, uint32_t sample_rate, reverb_preset preset, stereo_t volume, uint32_t sinc_window) :
    m_original_stream(nullptr), m_reverb_stream(nullptr)
{
    assert(source != nullptr);
    assert(sample_rate > 0);
    assert(preset >= 0 && preset < rp_number_of_presets);
    assert(preset != rp_off);

    // Split the source stream and use one copy as the original audio.
    m_original_stream.reset(new splitter_stereo(source));

    // Split another copy of the source stream for the reverb effect. When the
    // source is running at 22.05 kHz there is no need to resample. Otherwise we
    // need to resample to 22.05 kHz, generate the reverb effect, then resample
    // back to the source rate. Ideally the down-sampling phases would use
    // filtering to remove any frequencies above the target Nyquist limit, but
    // this takes out too much of the frequencies we want. Plus the lack of the
    // filter doesn't seem to introduce any artifacts.
    const double max_cut_off = 0.45;
    module_stereo *reverb_stream = m_original_stream->split();
    if (sample_rate != PSXDMH_REVERB_RATE)
    {
        if (sample_rate > PSXDMH_REVERB_RATE)
        {
            double cut_off = std::min(double(PSXDMH_REVERB_RATE) / sample_rate, max_cut_off);
            reverb_stream = new filter_stereo(reverb_stream, filter_type::low_pass, cut_off);
        }
        reverb_stream = new resampler_sinc_stereo(reverb_stream, sinc_window, sample_rate, PSXDMH_REVERB_RATE);
    }
    reverb_stream = new reverb_core(reverb_stream, preset, volume);
    if (sample_rate != PSXDMH_REVERB_RATE)
    {
        if (sample_rate < PSXDMH_REVERB_RATE)
        {
            double cut_off = std::min(double(sample_rate) / PSXDMH_REVERB_RATE, max_cut_off);
            reverb_stream = new filter_stereo(reverb_stream, filter_type::low_pass, cut_off);
        }
        reverb_stream = new resampler_sinc_stereo(reverb_stream, sinc_window, PSXDMH_REVERB_RATE, sample_rate);
    }
    m_reverb_stream.reset(reverb_stream);
}


//
// Test whether the module is still generating output.
//

bool
reverb::is_running() const
{
    return m_original_stream->is_running() || m_reverb_stream->is_running();
}


//
// Get the next sample.
//

bool
reverb::next(stereo_t &s)
{
    // Mix the reverb stream back into the original audio.
    bool original_live = m_original_stream->next(s);
    stereo_t r;
    bool reverb_live = m_reverb_stream->next(r);
    s += r;
    return original_live || reverb_live;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Registers for each preset.
const uint16_t reverb_core::m_registers[rp_number_of_presets][32] =
{
    // Off. Not used.
    {
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    },

    // Room.
    {
        0x007d, 0x005b, 0x6d80, 0x54b8, 0xbed0, 0x0000, 0x0000, 0xba80,
        0x5800, 0x5300, 0x04d6, 0x0333, 0x03f0, 0x0227, 0x0374, 0x01ef,
        0x0334, 0x01b5, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x01b4, 0x0136, 0x00b8, 0x005c, 0x8000, 0x8000
    },

    // Studio Small.
    {
        0x0033, 0x0025, 0x70f0, 0x4fa8, 0xbce0, 0x4410, 0xc0f0, 0x9c00,
        0x5280, 0x4ec0, 0x03e4, 0x031b, 0x03a4, 0x02af, 0x0372, 0x0266,
        0x031c, 0x025d, 0x025c, 0x018e, 0x022f, 0x0135, 0x01d2, 0x00b7,
        0x018f, 0x00b5, 0x00b4, 0x0080, 0x004c, 0x0026, 0x8000, 0x8000
    },

    // Studio Medium.
    {
        0x00b1, 0x007f, 0x70f0, 0x4fa8, 0xbce0, 0x4510, 0xbef0, 0xb4c0,
        0x5280, 0x4ec0, 0x0904, 0x076b, 0x0824, 0x065f, 0x07a2, 0x0616,
        0x076c, 0x05ed, 0x05ec, 0x042e, 0x050f, 0x0305, 0x0462, 0x02b7,
        0x042f, 0x0265, 0x0264, 0x01b2, 0x0100, 0x0080, 0x8000, 0x8000
    },

    // Studio Large.
    {
        0x00e3, 0x00a9, 0x6f60, 0x4fa8, 0xbce0, 0x4510, 0xbef0, 0xa680,
        0x5680, 0x52c0, 0x0dfb, 0x0b58, 0x0d09, 0x0a3c, 0x0bd9, 0x0973,
        0x0b59, 0x08da, 0x08d9, 0x05e9, 0x07ec, 0x04b0, 0x06ef, 0x03d2,
        0x05ea, 0x031d, 0x031c, 0x0238, 0x0154, 0x00aa, 0x8000, 0x8000
    },

    // Hall.
    {
        0x01a5, 0x0139, 0x6000, 0x5000, 0x4c00, 0xb800, 0xbc00, 0xc000,
        0x6000, 0x5c00, 0x15ba, 0x11bb, 0x14c2, 0x10bd, 0x11bc, 0x0dc1,
        0x11c0, 0x0dc3, 0x0dc0, 0x09c1, 0x0bc4, 0x07c1, 0x0a00, 0x06cd,
        0x09c2, 0x05c1, 0x05c0, 0x041a, 0x0274, 0x013a, 0x8000, 0x8000
    },

    // Half Echo.
    {
        0x0017, 0x0013, 0x70f0, 0x4fa8, 0xbce0, 0x4510, 0xbef0, 0x8500,
        0x5f80, 0x54c0, 0x0371, 0x02af, 0x02e5, 0x01df, 0x02b0, 0x01d7,
        0x0358, 0x026a, 0x01d6, 0x011e, 0x012d, 0x00b1, 0x011f, 0x0059,
        0x01a0, 0x00e3, 0x0058, 0x0040, 0x0028, 0x0014, 0x8000, 0x8000
    },

    // Space Echo.
    {
        0x033d, 0x0231, 0x7e00, 0x5000, 0xb400, 0xb000, 0x4c00, 0xb000,
        0x6000, 0x5400, 0x1ed6, 0x1a31, 0x1d14, 0x183b, 0x1bc2, 0x16b2,
        0x1a32, 0x15ef, 0x15ee, 0x1055, 0x1334, 0x0f2d, 0x11f6, 0x0c5d,
        0x1056, 0x0ae1, 0x0ae0, 0x07a2, 0x0464, 0x0232, 0x8000, 0x8000
    },
};


// Buffer size for each preset.
const size_t reverb_core::m_buffer_size[rp_number_of_presets] =
{
    0x00002 / sizeof(uint16_t), // Off.
    0x026c0 / sizeof(uint16_t), // Room.
    0x01f40 / sizeof(uint16_t), // Studio Small.
    0x04840 / sizeof(uint16_t), // Studio Medium.
    0x06fe0 / sizeof(uint16_t), // Studio Large.
    0x0ade0 / sizeof(uint16_t), // Hall.
    0x03c00 / sizeof(uint16_t), // Half Echo.
    0x0f6c0 / sizeof(uint16_t), // Space Echo.
};


//
// Construction.
//

reverb_core::reverb_core(module_stereo *source, reverb_preset preset, stereo_t volume) :
    module_stereo(source),
    m_preset(preset),
    m_volume(volume),
    m_buffer(m_buffer_size[preset], 0.0f), m_current(0),
    m_buffer_is_silent(false), m_last_unsilent_sample(0)
{
    assert(source != nullptr);
    assert(preset >= 0 && preset < rp_number_of_presets);
    assert(preset != rp_off);

    // Calculate the threshold of silence.
    mono_t max_volume = std::max(m_volume.left, m_volume.right);
    m_silence = PSXDMH_SILENCE / std::max(max_volume, mono_t(0.001));

    // Load a reverb configuration into the registers.
    m_dapf1 = reg_to_offset(m_registers[preset][0x00]);
    m_dapf2 = reg_to_offset(m_registers[preset][0x01]);
    m_viir = reg_to_volume(m_registers[preset][0x02]);
    m_vcomb1 = reg_to_volume(m_registers[preset][0x03]);
    m_vcomb2 = reg_to_volume(m_registers[preset][0x04]);
    m_vcomb3 = reg_to_volume(m_registers[preset][0x05]);
    m_vcomb4 = reg_to_volume(m_registers[preset][0x06]);
    m_vwall = reg_to_volume(m_registers[preset][0x07]);
    m_vapf1 = reg_to_volume(m_registers[preset][0x08]);
    m_vapf2 = reg_to_volume(m_registers[preset][0x09]);
    m_mlsame = reg_to_offset(m_registers[preset][0x0a]);
    m_mrsame = reg_to_offset(m_registers[preset][0x0b]);
    m_mlcomb1 = reg_to_offset(m_registers[preset][0x0c]);
    m_mrcomb1 = reg_to_offset(m_registers[preset][0x0d]);
    m_mlcomb2 = reg_to_offset(m_registers[preset][0x0e]);
    m_mrcomb2 = reg_to_offset(m_registers[preset][0x0f]);
    m_dlsame = reg_to_offset(m_registers[preset][0x10]);
    m_drsame = reg_to_offset(m_registers[preset][0x11]);
    m_mldiff = reg_to_offset(m_registers[preset][0x12]);
    m_mrdiff = reg_to_offset(m_registers[preset][0x13]);
    m_mlcomb3 = reg_to_offset(m_registers[preset][0x14]);
    m_mrcomb3 = reg_to_offset(m_registers[preset][0x15]);
    m_mlcomb4 = reg_to_offset(m_registers[preset][0x16]);
    m_mrcomb4 = reg_to_offset(m_registers[preset][0x17]);
    m_dldiff = reg_to_offset(m_registers[preset][0x18]);
    m_drdiff = reg_to_offset(m_registers[preset][0x19]);
    m_mlapf1 = reg_to_offset(m_registers[preset][0x1a]);
    m_mrapf1 = reg_to_offset(m_registers[preset][0x1b]);
    m_mlapf2 = reg_to_offset(m_registers[preset][0x1c]);
    m_mrapf2 = reg_to_offset(m_registers[preset][0x1d]);
    m_vlin = reg_to_volume(m_registers[preset][0x1e]);
    m_vrin = reg_to_volume(m_registers[preset][0x1f]);

    // Calculate derived addresses.
    m_mlsame_1 = wrap_offset(m_mlsame + m_buffer.size() - 1);
    m_mrsame_1 = wrap_offset(m_mrsame + m_buffer.size() - 1);
    m_mldiff_1 = wrap_offset(m_mldiff + m_buffer.size() - 1);
    m_mrdiff_1 = wrap_offset(m_mrdiff + m_buffer.size() - 1);
    m_mlapf1_dapf1 = wrap_offset(m_mlapf1 + m_buffer.size() - m_dapf1);
    m_mrapf1_dapf1 = wrap_offset(m_mrapf1 + m_buffer.size() - m_dapf1);
    m_mlapf2_dapf2 = wrap_offset(m_mlapf2 + m_buffer.size() - m_dapf2);
    m_mrapf2_dapf2 = wrap_offset(m_mrapf2 + m_buffer.size() - m_dapf2);
}


//
// Test whether the module is still generating output.
//

bool
reverb_core::is_running() const
{
    // Reverb is definitely running if the source is still running.
    if (source()->is_running())
    {
        return true;
    }

    // At this point the source has stopped running and we need to wait for the
    // reverb effect within the buffer to die down.
    if (m_buffer_is_silent)
    {
        return false;
    }

    // Look for a non-silent sample in the buffer. Start where the last such
    // sample was found.
    size_t start = m_last_unsilent_sample;
    do
    {
        if (magnitude(m_buffer[m_last_unsilent_sample]) > m_silence)
        {
            break;
        }
        if (++m_last_unsilent_sample >= m_buffer.size())
        {
            m_last_unsilent_sample = 0;
        }
    }
    while (start != m_last_unsilent_sample);
    m_buffer_is_silent = magnitude(m_buffer[m_last_unsilent_sample]) <= m_silence;
    return !m_buffer_is_silent;
}


//
// Get the next sample.
//

bool
reverb_core::next(stereo_t &s)
{
    // Get the next sample from the source.
    bool live = source()->next(s) || is_running();
    if (live)
    {
        // Apply volume to the input.
        mono_t lin = m_vlin * s.left;
        mono_t rin = m_vrin * s.right;

        // Same side reflection.
        const mono_t prev_mlsame = read_buffer(m_mlsame_1);
        const mono_t prev_mrsame = read_buffer(m_mrsame_1);
        write_buffer(m_mlsame, (lin + read_buffer(m_dlsame) * m_vwall - prev_mlsame) * m_viir + prev_mlsame);
        write_buffer(m_mrsame, (rin + read_buffer(m_drsame) * m_vwall - prev_mrsame) * m_viir + prev_mrsame);

        // Different side reflection.
        const mono_t prev_mldiff = read_buffer(m_mldiff_1);
        const mono_t prev_mrdiff = read_buffer(m_mrdiff_1);
        write_buffer(m_mldiff, (lin + read_buffer(m_drdiff) * m_vwall - prev_mldiff) * m_viir + prev_mldiff);
        write_buffer(m_mrdiff, (rin + read_buffer(m_dldiff) * m_vwall - prev_mrdiff) * m_viir + prev_mrdiff);

        // Early echo.
        mono_t lout = m_vcomb1 * read_buffer(m_mlcomb1) + m_vcomb2 * read_buffer(m_mlcomb2) + m_vcomb3 * read_buffer(m_mlcomb3) + m_vcomb4 * read_buffer(m_mlcomb4);
        mono_t rout = m_vcomb1 * read_buffer(m_mrcomb1) + m_vcomb2 * read_buffer(m_mrcomb2) + m_vcomb3 * read_buffer(m_mrcomb3) + m_vcomb4 * read_buffer(m_mrcomb4);

        // Late reverb all pass filter 1.
        lout -= m_vapf1 * read_buffer(m_mlapf1_dapf1);
        write_buffer(m_mlapf1, lout);
        lout = lout * m_vapf1 + read_buffer(m_mlapf1_dapf1);
        rout -= m_vapf1 * read_buffer(m_mrapf1_dapf1);
        write_buffer(m_mrapf1, rout);
        rout = rout * m_vapf1 + read_buffer(m_mrapf1_dapf1);

        // Late reverb all pass filter 2.
        lout -= m_vapf2 * read_buffer(m_mlapf2_dapf2);
        write_buffer(m_mlapf2, lout);
        lout = lout * m_vapf2 + read_buffer(m_mlapf2_dapf2);
        rout -= m_vapf2 * read_buffer(m_mrapf2_dapf2);
        write_buffer(m_mrapf2, rout);
        rout = rout * m_vapf2 + read_buffer(m_mrapf2_dapf2);

        // Apply volume to the output.
        s = flush_denorm(m_volume * stereo_t(lout, rout));

        // Advance the buffer position.
        if (++m_current >= m_buffer.size())
        {
            m_current = 0;
        }
    }
    return live;
}


}; //namespace psxdmh
