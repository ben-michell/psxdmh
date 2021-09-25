// psxdmh/src/volume.h
// Volume adjuster.
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

#ifndef PSXDMH_SRC_VOLUME_H
#define PSXDMH_SRC_VOLUME_H


#include "module.h"


namespace psxdmh
{


// Volume adjuster.
template <typename S> class volume : public module<S>
{
public:

    // Construction.
    volume(module<S> *source, mono_t level) :
        module<S>(source),
        m_level(level)
    {
        assert(source != nullptr);
    }

    // Test whether the module is still generating output.
    virtual bool is_running() const { return this->source()->is_running(); }

    // Get the next sample.
    virtual bool next(S &s)
    {
        bool live = this->source()->next(s);
        s *= m_level;
        return live;
    }

private:

    // Volume scaling.
    mono_t m_level;
};


// Types for mono and stereo volume adjusters.
typedef volume<mono_t> volume_mono;
typedef volume<stereo_t> volume_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_VOLUME_H
