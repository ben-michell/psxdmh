// psxdmh/src/endian.h
// Endianness detection and utilities.
// Copyright (c) 2021 Ben Michell <https://www.muryan.com/>. All rights reserved.

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

#ifndef PSXDMH_SRC_ENDIAN_H
#define PSXDMH_SRC_ENDIAN_H


namespace psxdmh
{


// Determine the endianness of the target.
#ifndef PSXDMH_LITTLE_ENDIAN
    #if defined(__LITTLE_ENDIAN__) || defined(_M_IX86) || defined(_M_X64)
        #define PSXDMH_LITTLE_ENDIAN
    #elif defined(__BIG_ENDIAN__)
        // Positively identified as targetting big endian.
    #else // Endian.
        #error Unknown endianness.
    #endif // Endian.
#endif // PSXDMH_LITTLE_ENDIAN


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Ensure an int16_t is in little-endian byte order.
inline int16_t int16_as_le(int16_t v)
{
#ifdef PSXDMH_LITTLE_ENDIAN
    return v;
#else // PSXDMH_LITTLE_ENDIAN
    return int16_t(((((uint16_t) v) << 8) & 0xff00) | ((((uint16_t) v) >> 8) & 0x00ff));
#endif // PSXDMH_LITTLE_ENDIAN
}


// Ensure a uint16_t is in little-endian byte order.
inline uint16_t uint16_as_le(uint16_t v)
{
#ifdef PSXDMH_LITTLE_ENDIAN
    return v;
#else // PSXDMH_LITTLE_ENDIAN
    return uint16_t(((v << 8) & 0xff00) | ((v >> 8) & 0x00ff));
#endif // PSXDMH_LITTLE_ENDIAN
}


// Ensure a uint32_t is in little-endian byte order.
inline uint32_t uint32_as_le(uint32_t v)
{
#ifdef PSXDMH_LITTLE_ENDIAN
    return v;
#else // PSXDMH_LITTLE_ENDIAN
    return ((v << 24) & 0xff000000) | ((v << 8) & 0x00ff0000) | ((v >> 8) & 0x0000ff00) | ((v >> 24) & 0x000000ff);
#endif // PSXDMH_LITTLE_ENDIAN
}


}; //namespace psxdmh


#endif // PSXDMH_SRC_ENDIAN_H

