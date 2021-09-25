// psxdmh/src/module.h
// Base class for all audio generation and processing modules.
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

#ifndef PSXDMH_SRC_MODULE_H
#define PSXDMH_SRC_MODULE_H


#include "sample.h"


namespace psxdmh
{


// Base class for all audio generation and processing modules. The S type is the
// sample type (mono_t or stereo_t).
template <typename S> class module : public uncopyable
{
public:

    // Construction. This object takes control of the source object if one is
    // provided. All derived classes must obey this convention of taking
    // ownership of source modules.
    module(module *source = nullptr) : m_source(source) {}

    // Virtualize the destructor.
    virtual ~module() {}

    // Get the source module. Returns nullptr if this module does not use a
    // source.
    virtual module *source() const { return m_source.get(); }

    // Test whether the module is still generating output.
    virtual bool is_running() const = 0;

    // Get the next sample. The return value indicates whether the module was
    // running. Once a module stops running this method must set s to 0 and
    // return false.
    virtual bool next(S &s) = 0;

private:

    // Source module. May be nullptr.
    std::unique_ptr<module> m_source;
};


// Types for mono and stereo modules.
typedef module<mono_t> module_mono;
typedef module<stereo_t> module_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_MODULE_H
