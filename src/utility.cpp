// psxdmh/src/utility.cpp
// Utility classes and functions.
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

#include "utility.h"


namespace psxdmh
{


//
// Number parsing with range checking.
//

long
string_to_long(const char *str, long min_value, long max_value, std::string name)
{
    assert(str != nullptr);
    long value;
    char guard;
    if (sscanf(str, " %ld %c", &value, &guard) != 1 || value < min_value || value > max_value)
    {
        throw std::string("Invalid value for ") + name + ".";
    }
    return value;
}


//
// Number parsing with range checking.
//

double
string_to_double(const char *str, double min_value, double max_value, std::string name)
{
    assert(str != nullptr);
    double value;
    char guard;
    if (sscanf(str, " %lf %c", &value, &guard) != 1 || value < min_value || value > max_value)
    {
        throw std::string("Invalid value for ") + name + ".";
    }
    return value;
}


//
// Parse a range specification into individual item numbers.
//

void
parse_range(std::string range, uint16_t limit, std::string item_name, std::vector<uint16_t> &parsed_range)
{
    assert(limit > 0);
    assert(!item_name.empty());
    assert(parsed_range.empty());
    size_t pos = 0;
    while (pos != range.npos)
    {
        // Process each comma-separated group.
        size_t comma = range.find(',', pos);
        std::string sub = range.substr(pos, comma-pos);

        // Handle hyphen-separated ranges.
        unsigned int start, end;
        char guard;
        bool valid;
        if (sub.find('-') != sub.npos)
        {
            valid = sscanf(sub.c_str(), " %u - %u %c", &start, &end, &guard) == 2;
        }
        // Handle single numbers.
        else
        {
            valid = sscanf(sub.c_str(), " %u %c", &start, &guard) == 1;
            end = start;
        }

        // Report errors.
        if (!valid || start > end)
        {
            throw std::string("Invalid ") + item_name + " number specification.";
        }
        if (end >= limit)
        {
            throw std::string("Invalid ") + item_name + " number " + int_to_string(end) + ".";
        }

        // Store the numbers.
        while (start <= end)
        {
            parsed_range.push_back((uint16_t) start);
            start++;
        }

        // Move to the next group.
        pos = comma;
        if (pos != range.npos)
        {
            pos++;
        }
    }
}


//
// Conversion of numbers to strings.
//

std::string
int_to_string(int value)
{
    char buffer[32];
    sprintf(buffer, "%d", value);
    return buffer;
}


//
// Convert a tick count to a time string.
//

std::string
ticks_to_time(uint32_t ticks, uint32_t sample_rate, int precision)
{
    assert(sample_rate > 0);
    assert(precision >= 0 && precision <= 3);
    uint32_t seconds = ticks / sample_rate;
    ticks %= sample_rate;
    static const uint32_t scale[4] = { 1, 10, 100, 1000 };
    uint32_t milliseconds = scale[precision] * ticks / sample_rate;
    char buffer[32];
    if (precision == 0)
    {
        sprintf(buffer, "%u:%02u", seconds / 60, seconds % 60);
    }
    else
    {
        sprintf(buffer, "%u:%02u.%0*u", seconds / 60, seconds % 60, precision, milliseconds);
    }
    return buffer;
}


//
// Dump memory as hex bytes.
//

std::string
hex_byte(uint8_t byte)
{
    char buffer[4];
    sprintf(buffer, "%02x", byte);
    return buffer;
}


//
// Dump memory as hex bytes.
//

std::string
hex_bytes(const uint8_t *ptr, size_t size)
{
    assert(ptr != nullptr);
    assert(size > 0);
    std::string hex = "";
    while (size-- > 0)
    {
        if (hex.length() > 0)
        {
            hex += " ";
        }
        hex += hex_byte(*ptr++);
    }
    return hex;
}


//
// Word wrap text.
//

std::string
word_wrap(std::string text, size_t indent, size_t width)
{
    // Process line by line.
    assert(indent < width);
    assert(width > 0);
    const size_t set_width = width - indent;
    std::string result;
    size_t pos = 0;
    while (pos < text.size())
    {
        // Skip initial whitespace.
        while (isspace(text[pos]) && text[pos] != '\n')
        {
            pos++;
        }

        // Accumulate the line word by word.
        size_t line_start = pos;
        size_t line_end = line_start;
        while (pos < text.size() && text[pos] != '\n')
        {
            // Skip initial whitespace.
            size_t word_end = pos;
            while (word_end < text.size() && isspace(text[word_end]) && text[word_end] != '\n')
            {
                word_end++;
            }

            // Find the end of the next word.
            while (word_end < text.size() && !isspace(text[word_end]))
            {
                word_end++;
            }

            // Check if the word will fit on the line. Always put at least one
            // word on a line, even if it doesn't fit.
            if ((word_end - line_start) <= set_width || line_start == line_end)
            {
                line_end = word_end;
                pos = word_end;
            }
            // The line is finished.
            else
            {
                break;
            }
        }

        // Add the line to the result.
        if (line_end > line_start)
        {
            if (!result.empty())
            {
                result += "\n";
            }
            result += std::string(indent, ' ') + std::string(text, line_start, line_end - line_start);
        }
        if (pos < text.size() && text[pos] == '\n')
        {
            pos++;
        }
    }
    return result;
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Retrieve the current time with high precision.
//

double
time_now()
{
    static auto base = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = now - base;
    return diff.count();
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Combine file system paths.
//

std::string
combine_paths(std::string path, std::string file)
{
    assert(!file.empty());
#if !defined(PSXDMH_TARGET_WINDOWS)
    const char path_separator = '/';
#else // Target.
    const char path_separator = '\\';
#endif // Target.

    if (!path.empty() && path.back() != path_separator)
    {
        path += path_separator;
    }
    return path + file;
}


//
// Determine the type of a file.
//

file_type
type_of_file(std::string file)
{
#if defined(PSXDMH_TARGET_WINDOWS)
    struct _stat st;
    return _stat(file.c_str(), &st) == 0 && (st.st_mode & _S_IFMT) == _S_IFDIR ? file_type::directory : file_type::file;
#else // Target.
    struct stat st;
    return stat(file.c_str(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR ? file_type::directory : file_type::file;
#endif // Target.
}


//
// Test if a file is interactive (a terminal).
//

bool
is_interactive(FILE *file)
{
#if defined(PSXDMH_TARGET_WINDOWS)
    return _isatty(_fileno(file)) != 0;
#else // Target.
    return isatty(fileno(file)) != 0;
#endif // Target.
}


}; //namespace psxdmh
