// psxdmh/src/sample.h
// Sample types for mono and stereo audio.
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

#ifndef PSXDMH_SRC_SAMPLE_H
#define PSXDMH_SRC_SAMPLE_H


#include "utility.h"


namespace psxdmh
{


// Sample magnitude for the threshold of silence.
#define PSXDMH_SILENCE          mono_t(1.0 / SHRT_MAX)


// Sample magnitude below which values are zeroed to prevent denorms.
#define PSXDMH_DENORM_LIMIT     (1e-9)


// Mono audio sample.
typedef float mono_t;


// Stereo pair of audio samples. This class can be used in much the same way as
// mono_t, which means templated code can be written that will work with either
// mono or stereo audio.
struct stereo_t
{
    // Stereo samples.
    mono_t left;
    mono_t right;

    // Construction.
    stereo_t() {}
    stereo_t(const stereo_t &v) : left(v.left), right(v.right) {}
    stereo_t(mono_t l, mono_t r) : left(l), right(r) {}
    stereo_t(mono_t v) : left(v), right(v) {}

    // Assignment.
    stereo_t &operator+=(const stereo_t &v) { left += v.left; right += v.right; return *this; }
    stereo_t &operator-=(const stereo_t &v) { left -= v.left; right -= v.right; return *this; }
    stereo_t &operator*=(const stereo_t &v) { left *= v.left; right *= v.right; return *this; }
    stereo_t &operator/=(const stereo_t &v) { left /= v.left; right /= v.right; return *this; }

    // Comparison.
    bool operator==(mono_t v) const { return left == v && right == v; }
    bool operator==(stereo_t v) const { return left == v.left && right == v.right; }
    bool operator!=(mono_t v) const { return left != v || right != v; }
    bool operator!=(stereo_t v) const { return left != v.left || right != v.right; }

    // Maths.
    stereo_t operator+(const stereo_t &s) const { return stereo_t(left + s.left, right + s.right); }
    stereo_t operator+(mono_t v) const { return stereo_t(left + v, right + v); }
    friend stereo_t operator+(mono_t v, const stereo_t &s) { return stereo_t(s.left + v, s.right + v); }
    stereo_t operator-(const stereo_t &s) const { return stereo_t(left - s.left, right - s.right); }
    stereo_t operator-(mono_t v) const { return stereo_t(left - v, right - v); }
    friend stereo_t operator-(mono_t v, const stereo_t &s) { return stereo_t(s.left - v, s.right - v); }
    stereo_t operator*(const stereo_t &s) const { return stereo_t(left * s.left, right * s.right); }
    stereo_t operator*(mono_t v) const { return stereo_t(left * v, right * v); }
    friend stereo_t operator*(mono_t v, const stereo_t &s) { return stereo_t(s.left * v, s.right * v); }
    stereo_t operator/(const stereo_t &s) const { return stereo_t(left / s.left, right / s.right); }
    stereo_t operator/(mono_t v) const { return stereo_t(left / v, right / v); }
    friend stereo_t operator/(mono_t v, const stereo_t &s) { return stereo_t(s.left / v, s.right / v); }

    // Math functions.
    friend stereo_t fabs(const stereo_t &s) { return stereo_t(::fabs(s.left), ::fabs(s.right)); }
};


// Test a sample for silence.
inline bool is_silent(mono_t s) { return ::fabs(s) < PSXDMH_SILENCE; }
inline bool is_silent(const stereo_t &s) { return ::fabs(s.left) < PSXDMH_SILENCE && ::fabs(s.right) < PSXDMH_SILENCE; }


// Clamp near-zero values to zero to prevent denormals. It's important to ensure
// that denormals (underflowing values) don't get into the calculations as they
// can cause a massive (orders of magnitude) decrease in performance.
inline mono_t flush_denorm(mono_t s) { return ::fabs(s) < PSXDMH_DENORM_LIMIT ? 0 : s; }
inline stereo_t flush_denorm(const stereo_t &s)
{
    return stereo_t(::fabs(s.left) < PSXDMH_DENORM_LIMIT ? 0 : s.left, ::fabs(s.right) < PSXDMH_DENORM_LIMIT ? 0 : s.right);
}


// Get the magnitude of a sample.
inline mono_t magnitude(mono_t sample) { return ::fabs(sample); }
inline mono_t magnitude(const stereo_t &sample) { return std::max(::fabs(sample.left), ::fabs(sample.right)); }


// Conversion of floating point samples to 16-bit integers.
inline int16_t sample_to_int(mono_t s)
{
    return int16_t(clamp(int32_t(s * SHRT_MAX + 0.5), SHRT_MIN, SHRT_MAX));
}
inline std::pair<int16_t, int16_t> sample_to_int(const stereo_t &s)
{
    return std::make_pair(int16_t(clamp(int32_t(s.left * SHRT_MAX + 0.5), SHRT_MIN, SHRT_MAX)), int16_t(clamp(int32_t(s.right * SHRT_MAX + 0.5), SHRT_MIN, SHRT_MAX)));
}


}; //namespace psxdmh


#endif // PSXDMH_SRC_SAMPLE_H
