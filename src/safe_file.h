// psxdmh/src/safe_file.h
// File I/O class with full error checking.
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

#ifndef PSXDMH_SRC_SAFE_FILE_H
#define PSXDMH_SRC_SAFE_FILE_H


#include "sample.h"


namespace psxdmh
{


// File access mode.
enum class file_mode
{
    read,
    write
};


// File I/O class with full error checking. All errors are reported by a thrown
// std::string.
class safe_file : public uncopyable
{
public:

    // Construction. Note that the constructor will throw an exception if it
    // encounters an error.
    safe_file(std::string file_name, file_mode mode);

    // Destruction.
    ~safe_file();

    // File name.
    std::string file_name() const { return m_file_name; }

    // Close the file. Although the destructor will close an open file, it is
    // better to call the close method explicitly so that any errors can be
    // reported.
    void close();

    // Size of the file in bytes.
    size_t size();

    // Test whether the file position is at the end. This is only valid for
    // files in read mode.
    bool eof();

    // Seek to a position relative to the start of the file.
    void seek(size_t pos);

    // Current position within the file.
    size_t tell();

    // Read bytes from the file.
    void read(void *buffer, size_t bytes);

    // Read a byte from the file.
    uint8_t read_8();

    // Read a 16-bit value from the file in little-endian order.
    uint16_t read_16_le();

    // Read a 32-bit value from the file in little-endian order.
    uint32_t read_32_le();

    // Read a sample in native byte order.
    void read_sample(mono_t &s);
    void read_sample(stereo_t &s);

    // Write bytes to the file.
    void write(const void *buffer, size_t bytes);

    // Write a byte to the file.
    void write_8(uint8_t value);

    // Write a 16-bit value to the file in little-endian order.
    void write_16_le(uint16_t value);

    // Write a 32-bit value to the file in little-endian order.
    void write_32_le(uint32_t value);

    // Write a sample in native byte order.
    void write_sample(const mono_t &s);
    void write_sample(const stereo_t &s);

    // Write a series of zero bytes to the file.
    void write_zeros(size_t count);

private:

    // Create error messages for read, write, and seek failure.
    std::string failed_reading() const { return std::string("Failed reading from '") + m_file_name + "'."; }
    std::string failed_writing() const { return std::string("Failed writing to '") + m_file_name + "'."; }
    std::string failed_seeking() const { return std::string("Failed seeking within '") + m_file_name + "'."; }

    // File handle.
    FILE *m_file;

    // File name and mode.
    std::string m_file_name;
    file_mode m_mode;

    // Size of the file. This is only set in read mode.
    size_t m_size;
};


}; // namespace psxdmh


#endif // PSXDMH_SRC_SAFE_FILE_H
