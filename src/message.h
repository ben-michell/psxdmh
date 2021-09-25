// psxdmh/src/message.h
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

#ifndef PSXDMH_SRC_MESSAGE_H
#define PSXDMH_SRC_MESSAGE_H


#include "utility.h"


namespace psxdmh
{


// Message output verbosity.
enum class verbosity
{
    quiet,    // Errors only.
    normal,   // Standard information.
    verbose   // Detailed information.
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Simple message output manager.
class message : public uncopyable
{
public:

    // Message output in the style of printf. The message will only be output if
    // v is less than or equal to the configured verbosity.
    static void writef(verbosity v, std::string format, ...);

    // Message output detail level.
    static void verbosity(enum verbosity v) { m_verbosity = v; }
    static enum verbosity verbosity() { return m_verbosity; }

private:

    // Constructor.
    message() {}

    // Output verbosity.
    static enum verbosity m_verbosity;
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_MESSAGE_H
