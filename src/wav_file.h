// psxdmh/src/wav_file.h
// WAV file writer.
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

#ifndef PSXDMH_SRC_WAV_FILE_H
#define PSXDMH_SRC_WAV_FILE_H


#include "endian.h"
#include "module.h"
#include "safe_file.h"
#include "sample.h"


namespace psxdmh
{


// WAV file writer. All errors are reported by a thrown std::string.
template <typename S> class wav_file : public uncopyable
{
public:

    // Construction.
    wav_file() :
        m_file(nullptr),
        m_riff_length_offset(0), m_data_length_offset(0),
        m_samples(0),
        m_max_samples((0xffffffffUL - m_header_size) / (sizeof(uint16_t) * (is_stereo() ? 2 : 1)))
    {
    }

    // Destruction.
    ~wav_file()
    {
        // Make sure the file is closed. Exceptions must be smothered as
        // destructors are not permitted to throw exceptions.
        try
        {
            close();
        }
        catch (...)
        {
        }
    }

    // Write the WAV file from source. The return value gives the number of
    // samples written.
    uint32_t write(module<S> *source, std::string file_name, uint32_t sample_rate)
    {
        // Open the file.
        assert(source != nullptr);
        assert(sample_rate > 0);
        assert(m_file == nullptr);
        open(file_name, sample_rate);

        // Extract and write all the source samples. Collect the samples in
        // batches to avoid excessive write calls to the file.
        const size_t buffer_samples = 4096;
        std::vector<int16_t> sample_buffer;
        S s;
        do
        {
            // Write a set of samples.
            sample_buffer.clear();
            while (sample_buffer.size() < buffer_samples && source->next(s))
            {
                buffer_sample(sample_buffer, s);
                if (++m_samples > m_max_samples)
                {
                    throw std::string("Maximum WAV file size exceeded.");
                }
            }
            if (!sample_buffer.empty())
            {
                m_file->write(sample_buffer.data(), sample_buffer.size() * sizeof(int16_t));
            }
        }
        while (!sample_buffer.empty());

        // Close the file. This patches the header to match the data written.
        close();
        return m_samples;
    }

    // Check if the file is open.
    bool is_file_open() const { return m_file != nullptr; }

    // Abort the writing of the WAV file.
    void abort()
    {
        if (m_file != nullptr)
        {
            // Ignore any exceptions.
            try
            {
                m_file.reset();
            }
            catch (...)
            {
            }

            // Remove the file.
            remove(m_file_name.c_str());
        }
    }

private:

    // Open the file.
    void open(std::string file_name, uint32_t sample_rate)
    {
        // Open the file.
        assert(m_file == nullptr);
        m_file_name = file_name;
        m_file.reset(new safe_file(m_file_name.c_str(), file_mode::write));

        // Write the file header. First the RIFF chunk. The size field needs to
        // be patched later.
        m_file->write("RIFF", 4);
        m_riff_length_offset = m_file->tell();
        m_file->write_32_le(0);

        // Write the WAVE chunk: PCM, mono/stereo, sample rate, 16 bits/sample.
        m_file->write("WAVEfmt ", 8);
        m_file->write_32_le(16);
        m_file->write_16_le(1);
        m_file->write_16_le(is_stereo() ? 2 : 1);
        m_file->write_32_le(sample_rate);
        size_t channels = is_stereo() ? 2 : 1;
        m_file->write_32_le(uint32_t(sizeof(uint16_t) * sample_rate * channels));
        m_file->write_16_le(uint16_t(sizeof(uint16_t) * channels));
        m_file->write_16_le(16);

        // Write the data chunk header. The size field needs to be patched
        // later.
        m_file->write("data", 4);
        m_data_length_offset = m_file->tell();
        m_file->write_32_le(0);
    }

    // Close the file and patch the header.
    void close()
    {
        // Patch the file and data lengths.
        if (m_file != nullptr)
        {
            m_file->seek(m_riff_length_offset);
            size_t sample_bytes = m_samples * sizeof(int16_t) * (is_stereo() ? 2 : 1);
            m_file->write_32_le(m_wave_chunk_size + m_data_chunk_size + uint32_t(sample_bytes));
            m_file->seek(m_data_length_offset);
            m_file->write_32_le(uint32_t(sample_bytes));
            m_file.reset();
        }
    }

    // Whether the audio is mono or stereo.
    bool is_stereo() const
    {
        assert(sizeof(S) == sizeof(mono_t) || sizeof(S) == sizeof(stereo_t));
        return sizeof(S) == sizeof(stereo_t);
    }

    static void buffer_sample(std::vector<int16_t> &buffer, mono_t s)
    {
        buffer.push_back(int16_as_le(sample_to_int(s)));
    }

    static void buffer_sample(std::vector<int16_t> &buffer, stereo_t s)
    {
        buffer.push_back(int16_as_le(sample_to_int(s.left)));
        buffer.push_back(int16_as_le(sample_to_int(s.right)));
    }

    // Name of the file.
    std::string m_file_name;

    // File wrapper.
    std::unique_ptr<safe_file> m_file;

    // Offsets to length fields in the file.
    size_t m_riff_length_offset;
    size_t m_data_length_offset;

    // Number of samples written to the file.
    uint32_t m_samples;

    // Number of bytes in the WAV file header, RIFF chunk, and WAVE chunk.
    static const uint32_t m_header_size;
    static const uint32_t m_wave_chunk_size;
    static const uint32_t m_data_chunk_size;

    // Maximum number of samples allowed (maximum size of a WAV file is 4 GB).
    const uint32_t m_max_samples;
};


// Number of bytes in the WAV file header.
template <typename S> const uint32_t wav_file<S>::m_header_size = 44;
template <typename S> const uint32_t wav_file<S>::m_wave_chunk_size = 28;
template <typename S> const uint32_t wav_file<S>::m_data_chunk_size = 8;


// Types for mono and stereo WAV file writers.
typedef wav_file<mono_t> wav_file_mono;
typedef wav_file<stereo_t> wav_file_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_WAV_FILE_H
