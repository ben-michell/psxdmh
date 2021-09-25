// psxdmh/src/utility.h
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

#ifndef PSXDMH_SRC_UTILITY_H
#define PSXDMH_SRC_UTILITY_H


namespace psxdmh
{


// Base class for classes that want to prevent copying.
class uncopyable
{
public:

    // Allow regular construction.
    uncopyable() {}

    // Disable copy construction and assignment.
    uncopyable(const uncopyable &) = delete;
    uncopyable &operator=(const uncopyable &) = delete;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Clamp a value to [min_value, max_value].
template<typename T>
T
clamp(T value, T min_value, T max_value)
{
    if (value <= min_value)
    {
        return min_value;
    }
    return value >= max_value ? max_value : value;
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Convert between decibels and amplitude.
inline double decibels_to_amplitude(double db) { return pow(10, db / 20); }
inline double amplitude_to_decibels(double amp) { return 20 * log10(amp); }


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Number parsing with range checking.
extern long string_to_long(const char *str, long min_value, long max_value, std::string name);
inline long string_to_long(std::string str, long min_value, long max_value, std::string name) { return string_to_long(str.c_str(), min_value, max_value, name); }
extern double string_to_double(const char *str, double min_value, double max_value, std::string name);
inline double string_to_double(std::string str, double min_value, double max_value, std::string name) { return string_to_double(str.c_str(), min_value, max_value, name); }


// Parse a range specification into individual item numbers.
extern void parse_range(std::string range, uint16_t limit, std::string item_name, std::vector<uint16_t> &parsed_range);


// Conversion of numbers to strings.
extern std::string int_to_string(int value);


// Convert a tick count to a time string. The precision gives the number of
// digits after the seconds, and must be in the range [0, 3].
extern std::string ticks_to_time(uint32_t ticks, uint32_t sample_rate, int precision = 3);


// Dump memory as hex bytes.
extern std::string hex_byte(uint8_t byte);
extern std::string hex_bytes(const uint8_t *ptr, size_t size);


// Word wrap text.
extern std::string word_wrap(std::string text, size_t indent, size_t width);


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Types of files.
enum class file_type
{
    file,
    directory
};


// Combine file system paths.
extern std::string combine_paths(std::string path, std::string file);


// Determine the type of a file.
extern file_type type_of_file(std::string file);


// Test if a file is interactive (a terminal).
extern bool is_interactive(FILE *file);


}; //namespace psxdmh


#endif // PSXDMH_SRC_UTILITY_H
