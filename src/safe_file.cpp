// psxdmh/src/safe_file.cpp
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

#include "global.h"

#include "endian.h"
#include "safe_file.h"


namespace psxdmh
{


//
// Construction.
//

safe_file::safe_file(std::string file_name, file_mode mode) :
    m_file_name(file_name), m_mode(mode),
    m_size(0)
{
    // Attempt to open the file.
    m_file = fopen(file_name.c_str(), mode == file_mode::write ? "wb" : "rb");
    if (m_file == nullptr)
    {
        throw std::string("Unable to open '") + m_file_name + "' for " + (mode == file_mode::write ? "writing." : "reading.");
    }

    // Get the length of the file in read mode.
    if (mode == file_mode::read)
    {
        m_size = size();
    }
}


//
// Destruction.
//

safe_file::~safe_file()
{
    // Make sure the file is closed. Exceptions must be smothered as destructors
    // are not permitted to throw exceptions.
    try
    {
        close();
    }
    catch (...)
    {
    }
}


//
// Close the file.
//

void
safe_file::close()
{
    if (m_file != nullptr)
    {
        bool bad = fclose(m_file) != 0;
        m_file = nullptr;
        if (bad)
        {
            throw std::string("Failed closing '") + m_file_name + "'.";
        }
    }
}


//
// Size of the file in bytes.
//

size_t
safe_file::size()
{
    // Remember the current position.
    assert(m_file != nullptr);
    size_t pos = tell();

    // Seek to the end of the file and get the offset.
    if (fseek(m_file, 0, SEEK_END) != 0)
    {
        throw failed_seeking();
    }
    size_t length = tell();

    // Restore the position.
    seek(pos);
    return length;
}


//
// Test whether the file position is at the end.
//

bool
safe_file::eof()
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::read);
    return tell() == m_size;
}


//
// Seek to a position relative to the start of the file.
//

void
safe_file::seek(size_t pos)
{
    assert(m_file != nullptr);
    if (fseek(m_file, (long) pos, SEEK_SET) != 0)
    {
        throw failed_seeking();
    }
}


//
// Current position within the file.
//

size_t
safe_file::tell()
{
    // Get the position.
    assert(m_file != nullptr);
    long pos = ftell(m_file);
    if (pos < 0)
    {
        throw std::string("Failed getting position within '") + m_file_name + "'.";
    }
    return size_t(pos);
}


//
// Read bytes from the file.
//

void
safe_file::read(void *buffer, size_t bytes)
{
    assert(m_file != nullptr);
    assert(buffer != 0);
    if (fread(buffer, 1, bytes, m_file) != bytes)
    {
        throw failed_reading();
    }
}


//
// Read a byte from the file.
//

uint8_t
safe_file::read_8()
{
    assert(m_file != nullptr);
    int c = fgetc(m_file);
    if (c == EOF)
    {
        throw failed_reading();
    }
    return (uint8_t) (c & 0xff);
}


//
// Read a 16-bit value from the file in little-endian order.
//

uint16_t
safe_file::read_16_le()
{
    assert(m_file != nullptr);
    uint16_t temp;
    if (fread(&temp, 1, sizeof(temp), m_file) != sizeof(temp))
    {
        throw failed_reading();
    }
    return uint16_as_le(temp);
}


//
// Read a 32-bit value from the file in little-endian order.
//

uint32_t
safe_file::read_32_le()
{
    assert(m_file != nullptr);
    uint32_t temp;
    if (fread(&temp, 1, sizeof(temp), m_file) != sizeof(temp))
    {
        throw failed_reading();
    }
    return uint32_as_le(temp);
}


//
// Read a sample in native byte order.
//

void
safe_file::read_sample(mono_t &s)
{
    assert(m_file != nullptr);
    if (fread(&s, 1, sizeof(s), m_file) != sizeof(s))
    {
        throw failed_reading();
    }
}


//
// Read a sample in native byte order.
//

void
safe_file::read_sample(stereo_t &s)
{
    assert(m_file != nullptr);
    read_sample(s.left);
    read_sample(s.right);
}


//
// Write bytes to the file.
//

void
safe_file::write(const void *buffer, size_t bytes)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    if (fwrite(buffer, 1, bytes, m_file) != bytes)
    {
        throw failed_writing();
    }
}


//
// Write a byte to the file.
//

void
safe_file::write_8(uint8_t value)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    if (fputc(value, m_file) == EOF)
    {
        throw failed_writing();
    }
}


//
// Write a 16-bit value to the file in little-endian order.
//

void
safe_file::write_16_le(uint16_t value)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    value = uint16_as_le(value);
    if (fwrite(&value, 1, sizeof(value), m_file) != sizeof(value))
    {
        throw failed_writing();
    }
}


//
// Write a 32-bit value to the file in little-endian order.
//

void
safe_file::write_32_le(uint32_t value)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    value = uint32_as_le(value);
    if (fwrite(&value, 1, sizeof(value), m_file) != sizeof(value))
    {
        throw failed_writing();
    }
}


//
// Write a sample in native byte order.
//

void
safe_file::write_sample(const mono_t &s)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    if (fwrite(&s, 1, sizeof(s), m_file) != sizeof(s))
    {
        throw failed_writing();
    }
}


//
// Write a sample in native byte order.
//

void
safe_file::write_sample(const stereo_t &s)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    write_sample(s.left);
    write_sample(s.right);
}


//
// Write a series of zero bytes to the file.
//

void
safe_file::write_zeros(size_t count)
{
    assert(m_file != nullptr);
    assert(m_mode == file_mode::write);
    while (count-- > 0)
    {
        if (fputc(0, m_file) == EOF)
        {
            throw failed_writing();
        }
    }
}


}; // namespace psxdmh
