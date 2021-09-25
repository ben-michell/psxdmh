// psxdmh/src/channel.cpp
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

#include "global.h"

#include "adpcm.h"
#include "channel.h"
#include "filter.h"
#include "lcd_file.h"


namespace psxdmh
{


// Filter cut off used to filter patches when decoded from ADPCM.
const double channel::m_adpcm_filter_cutoff = 0.33;


// Filtering fixes for noisy patches.
const channel::filter_fix channel::m_filter_fixes[] =
{
    { 104, 0.15 }, // Song 98.
    { 112, 0.15 }, // Song 102. Note that duplicates of this patch are used in
                   // other songs, however in them the noise isn't apparent so
                   // I haven't added fixes for them. These duplicates are
                   // song/patch 91/91, 97/103, 101/109, 108/119, and 109/121.
    { 128, 0.20 }, // Song 113.
    { 130, 0.20 }  // Song 114.
};


// Current and maximum number of channels instantiated simultaneously.
int channel::m_current_channels = 0;
int channel::m_maximum_channels = 0;


//
// Construction.
//

channel::channel(const patch *patch, uint32_t frequency, mono_t volume, uint8_t pan, uint16_t spu_ads, uint16_t spu_sr, uint32_t sample_rate, uint32_t sinc_window, bool apply_psx_limit, bool repair) :
    m_resampler(nullptr),
    m_raw_envelope(new envelope(spu_ads, spu_sr)), m_envelope(nullptr),
    m_pan(pan),
    m_volume(0.0),
    m_limit_frequency(apply_psx_limit),
    m_sinc_window(sinc_window),
    m_user_data(0)
{
    assert(patch != nullptr);
    assert(frequency > 0);
    assert(volume >= 0.0);
    assert(pan <= 0x7f);

    // Monitor the maximum number of channels in use simultaneously.
    assert(m_current_channels >= 0);
    if (++m_current_channels > m_maximum_channels)
    {
        m_maximum_channels = m_current_channels;
    }

    // Prepare the resampler. The output of the ADPCM decoder is filtered before
    // resampling to reduce artifacts from low quality patches. Doing this now
    // gives better results than trying to do it after resampling (and is
    // considerably easier to manage). One patch used in song 98 has a special
    // fix to remove high-pitched noise.
    module_mono *module = new adpcm(patch->adpcm);
    double cutoff = m_adpcm_filter_cutoff;
    if (repair)
    {
        for (size_t f = 0; f < numberof(m_filter_fixes); ++f)
        {
            if (m_filter_fixes[f].id == patch->id)
            {
                cutoff = m_filter_fixes[f].cutoff;
                break;
            }
        }
    }
    module = new filter_mono(module, filter_type::low_pass, cutoff);
    resampler_mono *resampler = new resampler_sinc_mono(module, m_sinc_window, limit_frequency(frequency), sample_rate);
    m_resampler.reset(resampler);

    // Calculate the left and right volumes.
    master_volume(volume);

    // Resample the envelope if its sample rate does not match ours. A linear
    // resampler is not good for regular audio but is fine here since the
    // envelope's output is quite linear in character and will not overshoot or
    // undershoot, unlike fancier resamplers.
    if (sample_rate != m_raw_envelope->sample_rate())
    {
        m_envelope.reset(new resampler_linear_mono(m_raw_envelope, m_raw_envelope->sample_rate(), sample_rate));
    }
    // Otherwise use the envelope directly.
    else
    {
        m_envelope.reset(m_raw_envelope);
    }
}


//
// Destruction.
//

channel::~channel()
{
    assert(m_current_channels > 0);
    m_current_channels--;
}


//
// Get the next sample.
//

bool
channel::next(stereo_t &s)
{
    // Return silence when not running.
    if (m_resampler == nullptr)
    {
        s = 0.0;
        return false;
    }

    // Get the waveform at the current position and scale it by the envelope and
    // channel volume.
    mono_t waveform, envelope;
    bool resampler_live = m_resampler->next(waveform);
    bool envelope_live = m_envelope->next(envelope);
    s = waveform * envelope * m_volume;

    // Stop the channel when either the resampler or envelope stops, as their
    // combined output is guaranteed to be 0 from then on.
    if (!resampler_live || !envelope_live)
    {
        m_resampler.reset();
    }
    return true;
}


//
// Set the master volume for the channel and calculate the left and right
// volumes.
//

void
channel::master_volume(mono_t volume)
{
    // MIDI's pan controls the left and right volumes with cosine and sine,
    // which preserves the same apparent volume as a sound moves around. See
    // MIDI Recommended Practice RP-036 (Default Pan Curve). In contrast the
    // sound player in PSX Doom seems to use a simple linear blend.
    m_volume.left = volume * (128 - m_pan) / 128.0f;
    m_volume.right = volume * (m_pan + 1) / 128.0f;
}


//
// Alter the playback frequency of the patch currently being played.
//

void
channel::frequency(uint32_t new_frequency)
{
    if (m_resampler != nullptr)
    {
        m_resampler->rate_in(limit_frequency(new_frequency));
    }
}


//
// Limit a frequency to the allowed range.
//

uint32_t
channel::limit_frequency(uint32_t frequency) const
{
    // Apply limits: never drop below 1 Hz, and never go above the maximum
    // possible SPU frequency unless allowed to.
    if (frequency == 0)
    {
        frequency = 1;
    }
    else if (frequency > spu_max_frequency() && m_limit_frequency)
    {
        frequency = spu_max_frequency();
    }
    return frequency;
}


}; //namespace psxdmh
