// psxdmh/src/channel.h
// PSX SPU channel emulation.
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

#ifndef PSXDMH_SRC_CHANNEL_H
#define PSXDMH_SRC_CHANNEL_H


#include "envelope.h"
#include "module.h"
#include "resampler.h"


namespace psxdmh
{


// Forwards.
class envelope;
struct patch;


// PSX SPU channel emulation.
class channel : public module_stereo
{
public:

    // Construction. The channel starts playing immediately. The volume ranges
    // from 0.0 to 1.0. The pan ranges from full left at 0x00 to centre at 0x40
    // to full right at 0x7f.
    channel(const patch *patch, uint32_t frequency, mono_t volume, uint8_t pan, uint16_t spu_ads, uint16_t spu_sr, uint32_t sample_rate, uint32_t sinc_window, bool apply_psx_limit, bool repair);

    // Destruction.
    virtual ~channel();

    // Test if the channel is running. Once started, the channel will run until
    // either the envelope finishes (after being released) or the end of the
    // patch is reached (for non-repeating patches).
    virtual bool is_running() const { return m_resampler != nullptr; }

    // Get the next sample.
    virtual bool next(stereo_t &stereo);

    // Set the master volume for the channel. The volume ranges from 0.0 to 1.0.
    void master_volume(mono_t volume);

    // Start the release phase of the envelope.
    void release() { m_raw_envelope->release(); }

    // Alter the playback frequency of the patch currently being played.
    void frequency(uint32_t new_frequency);

    // Access to a user-defined value.
    uint32_t user_data() const { return m_user_data; }
    void user_data(uint32_t value) { m_user_data = value; }

    // Maximum playback frequency of the PSX SPU.
    static uint32_t spu_max_frequency() { return 4 * 44100; }

    // Maximum number of channels instantiated simultaneously.
    static int maximum_channels() { return m_maximum_channels; }
    static void reset_maximum_channels() { m_maximum_channels = 0; }

private:

    // Details of filtering fixes to apply to patches.
    struct filter_fix
    {
       // Patch ID.
       uint16_t id;

       // Filter cutoff.
       double cutoff;
    };

    // Limit a frequency to the allowed range.
    uint32_t limit_frequency(uint32_t frequency) const;

    // Patch resampler.
    std::unique_ptr<resampler_mono> m_resampler;

    // Envelope used to control the sound.
    envelope *m_raw_envelope;

    // When the channel is being run at 44.1 kHz this points to the same object
    // as m_raw_envelope. However, at other sample rates this points to a
    // resampler which adapts the envelope (which is 44.1 kHz only) to the
    // sample rate is being used.
    std::unique_ptr<module_mono> m_envelope;

    // Panning. Full left is 0x00, centre is 0x40, and full right is 0x7f.
    uint8_t m_pan;

    // Left and right volumes for the channel.
    stereo_t m_volume;

    // Whether to limit the maximum playback frequency as on a real PSX.
    bool m_limit_frequency;

    // Resampling configuration.
    uint32_t m_sinc_window;

    // User-defined value.
    uint32_t m_user_data;

    // Filter cut off used to filter patches when decoded from ADPCM.
    static const double m_adpcm_filter_cutoff;

    // Filtering fixes for noisy patches.
    static const filter_fix m_filter_fixes[];

    // Current and maximum number of channels instantiated simultaneously.
    static int m_current_channels;
    static int m_maximum_channels;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_CHANNEL_H
