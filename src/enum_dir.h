// psxdmh/src/enum_dir.h
// Directory enumeration.
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

#ifndef PSXDMH_SRC_ENUM_DIR_H
#define PSXDMH_SRC_ENUM_DIR_H


#include "utility.h"


namespace psxdmh
{


// Enumeration of a directory. All errors are reported by a thrown std::string.
class enum_dir : public uncopyable
{
public:

    // Construction.
    enum_dir(std::string dir);

    // Destruction.
    ~enum_dir();

    // Get the next file or sub-directory. The return value is true if an entry
    // was returned, or false if there are no more entries. The contents of
    // directories may be returned in any order. Both files and sub-directories
    // are returned, with the exception of the current and parent directory
    // entries and (on macOS) cruft starting with "._". Sub-directories are not
    // recursively enumerated.
    bool next(std::string &name, file_type &type);

private:

    // Directory being enumerated.
    std::string m_dir;

#if defined(PSXDMH_TARGET_MACOS)

    // Directory stream.
    DIR *m_dir_stream;

#elif defined(PSXDMH_TARGET_WINDOWS)

    // Handle to the search.
    HANDLE m_search_handle;

    // Data about the current file or directory.
    WIN32_FIND_DATA m_find_data;
    bool m_first_entry;

#else // Target.

    #error Unsupported target platform.

#endif // Target.
};


}; //namespace psxdmh


#endif // PSXDMH_SRC_ENUM_DIR_H
