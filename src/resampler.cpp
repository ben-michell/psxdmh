// psxdmh/src/resampler.cpp
// Resampling of audio to arbitrary frequencies.
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

#include "resampler.h"


namespace psxdmh
{


// Private list of cached tables.
sinc_table *sinc_table::m_cache = nullptr;


//
// Obtain a table of sinc values.
//

const sinc_table &
sinc_table::obtain(uint32_t window, uint32_t rate_out)
{
    // Look for a cached table.
    sinc_table *table;
    for (table = m_cache; table != nullptr; table = table->m_next)
    {
        if (table->window() == window && table->rate_out() == rate_out)
        {
            return *table;
        }
    }

    // Create a new table.
    table = new sinc_table(window, rate_out);
    table->m_next = m_cache;
    m_cache = table;
    return *table;
}


//
// Construction.
//

sinc_table::sinc_table(uint32_t window, uint32_t rate_out) :
    m_next(nullptr),
    m_window(window), m_rate_out(rate_out)
{
    // Compute the table values, covering the range (-pi * window, pi * window].
    //
    // It would be natural to organize these values in a simple linear fashion,
    // however as these values are accessed by striding through them (by
    // m_rate_out) this means the memory accesses would be widely dispersed.
    // This would cause CPU cache misses which in turn would have a detrimental
    // effect on performance.
    //
    // Instead the values are grouped together according to how they're used:
    // the starting offset is used to group the values that follow when striding
    // through the sinc curve.
    //
    // The table value at pos=0 is 1.0, and the rest are given by:
    //   sinc(x).sinc(x/a)
    // which becomes:
    //   a.sin(pi.x).sin(pi.x/a) / (pi^2.x^2)
    assert(m_window >= 1);
    assert(m_rate_out > 0);
    m_table.resize(m_rate_out * m_window * 2);
    const int32_t base_pos = -int32_t((window - 1) * m_rate_out);
    size_t index = 0;
    for (int32_t offset = 0; offset < (int32_t) m_rate_out; ++offset)
    {
        assert(index == index_for_offset(offset));
        int32_t pos = base_pos - offset;
        const int32_t end_pos = pos + m_rate_out * m_window * 2;
        const double scale = M_PI / m_rate_out;
        for (; pos < end_pos; pos += m_rate_out)
        {
            if (pos != 0)
            {
                const double pi_x = scale * pos;
                m_table[index++] = flush_denorm(mono_t(m_window * sin(pi_x) * sin(pi_x / m_window) / (pi_x * pi_x)));
            }
            else
            {
                m_table[index++] = 1.0;
            }
        }
    }
    assert(index == m_table.size());
}


}; //namespace psxdmh
