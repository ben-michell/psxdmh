// psxdmh/src/message.cpp
// Simple message output manager.
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

#include "message.h"


namespace psxdmh
{


// Output verbosity.
verbosity message::m_verbosity = verbosity::normal;


//
// Message output in the style of printf.
//

void
message::writef(enum verbosity v, std::string format, ...)
{
    if (v <= m_verbosity)
    {
        va_list vl;
        va_start(vl, format);
        vprintf(format.c_str(), vl);
        fflush(stdout);
        va_end(vl);
    }
}


}; //namespace psxdmh
