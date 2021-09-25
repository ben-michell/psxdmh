// psxdmh/src/normalizer.h
// Level normalization.
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

#ifndef PSXDMH_SRC_NORMALIZER_H
#define PSXDMH_SRC_NORMALIZER_H


#include "module.h"
#include "safe_file.h"


namespace psxdmh
{


// Level normalization. This module adjusts the level of the audio that passes
// through it so that the highest amplitude is remapped to unity. This is done
// by first buffering the entire output of its source module in a temporary
// file. Note that the amount of temporary space required is twice that of the
// final file.
template <typename S> class normalizer : public module<S>
{
public:

    // Construction.
    normalizer(module<S> *source, std::string temp_name, double normalization_limit = 30.0) :
        module<S>(source),
        m_temp_file_name(temp_name),
        m_temp_file_created(false),
        m_temp_file(nullptr),
        m_normalization(mono_t(decibels_to_amplitude(normalization_limit))),
        m_samples(0), m_current_sample(0)
    {
        assert(source != 0);
    }

    // Destruction.
    virtual ~normalizer()
    {
        // Clean up the temporary file.
        if (m_temp_file != nullptr)
        {
            m_temp_file->close();
        }
        if (m_temp_file_created)
        {
            remove(m_temp_file_name.c_str());
        }
    }

    // Test whether the module is still generating output.
    virtual bool is_running() const
    {
        return m_current_sample < m_samples || this->source()->is_running();
    }

    // Get the next sample.
    virtual bool next(S &s)
    {
        // The first call buffers the source in the temporary file.
        if (m_temp_file == nullptr)
        {
            // Write the temporary file and track the maximum level.
            assert(m_normalization > 0.0);
            mono_t max_level = mono_t(1.0 / m_normalization);
            m_temp_file.reset(new safe_file(m_temp_file_name, file_mode::write));
            m_temp_file_created = true;
            S sample;
            while (this->source()->next(sample))
            {
                m_temp_file->write_sample(sample);
                m_samples++;
                max_level = std::max(max_level, magnitude(sample));
            }
            m_temp_file->close();
            m_temp_file.reset();

            // Calculate the normalization level.
            assert(max_level > 0.0);
            m_normalization = 1 / max_level;

            // Open the temporary file for reading.
            m_temp_file.reset(new safe_file(m_temp_file_name, file_mode::read));
        }

        // Return 0 past the end of the data.
        if (m_current_sample >= m_samples)
        {
            s = 0;
            return false;
        }

        // Read from the file and adjust the level.
        m_current_sample++;
        assert(m_temp_file != nullptr);
        m_temp_file->read_sample(s);
        s *= m_normalization;
        return true;
    }

    // Applied adjustment in dB.
    double adjustment_db() const
    {
        return amplitude_to_decibels(m_normalization);
    }

private:

    // Name to use for the temporary file.
    std::string m_temp_file_name;

    // Whether the temporary file was created.
    bool m_temp_file_created;

    // Temporary file containing the buffered audio.
    std::unique_ptr<safe_file> m_temp_file;

    // Normalization factor.
    mono_t m_normalization;

    // Total number of samples buffered.
    uint32_t m_samples;

    // Next sample to be extracted.
    uint32_t m_current_sample;
};


// Types for mono and stereo normalizers.
typedef normalizer<mono_t> normalizer_mono;
typedef normalizer<stereo_t> normalizer_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_NORMALIZER_H
