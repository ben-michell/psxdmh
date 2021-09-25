// psxdmh/src/enum_dir.cpp
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

#include "global.h"

#include "enum_dir.h"


namespace psxdmh
{


#if defined(PSXDMH_TARGET_MACOS)

//
// Construction.
//

enum_dir::enum_dir(std::string dir) :
    m_dir(dir)
{
    // Start the enumeration.
    m_dir_stream = opendir(m_dir.c_str());
    if (m_dir_stream == nullptr)
    {
        throw std::string("Error enumerating '") + m_dir + "'.";
    }
}


//
// Destruction.
//

enum_dir::~enum_dir()
{
    if (m_dir_stream != nullptr)
    {
        closedir(m_dir_stream);
    }
}


//
// Get the next file or sub-directory.
//

bool
enum_dir::next(std::string &name, file_type &type)
{
    // Stop if there is nothing left.
    if (m_dir_stream == nullptr)
    {
        return false;
    }

    // Loop to skip entries we don't like: current and parent directories, and
    // macOS cruft files (those starting with "._").
    dirent *entry;
    do
    {
        // Grab the details of the next entry.
        entry = readdir(m_dir_stream);

        // Stop if there is nothing left.
        if (entry == nullptr)
        {
            closedir(m_dir_stream);
            m_dir_stream = nullptr;
            return false;
        }
    }
    while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0
        || (entry->d_name[0] == '.' && entry->d_name[1] == '_'));

    // Return the details. When reading real game CD directories the type may be
    // set to DT_UNKNOWN, so we need to double-check these with stat.
    name = entry->d_name;
    type = entry->d_type == DT_DIR ? file_type::directory : file_type::file;
    if (entry->d_type == DT_UNKNOWN)
    {
        std::string full_name = combine_paths(m_dir, entry->d_name);
        struct stat st;
        type = stat(full_name.c_str(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR ? file_type::directory : file_type::file;
    }
    return true;
}

#elif defined(PSXDMH_TARGET_WINDOWS)

//
// Construction.
//

enum_dir::enum_dir(std::string dir) :
    m_dir(dir),
    m_first_entry(true)
{
    // Enumerate "*" in the given directory.
    std::string spec = combine_paths(dir, "*");
    m_search_handle = FindFirstFile(spec.c_str(), &m_find_data);
    if (m_search_handle == INVALID_HANDLE_VALUE && GetLastError() != ERROR_NO_MORE_FILES)
    {
        throw std::string("Error enumerating '") + m_dir + "'.";
    }
}


//
// Destruction.
//

enum_dir::~enum_dir()
{
    if (m_search_handle != INVALID_HANDLE_VALUE)
    {
        FindClose(m_search_handle);
    }
}


//
// Get the next file or sub-directory.
//

bool
enum_dir::next(std::string &name, file_type &type)
{
    // Stop if there is nothing left.
    if (m_search_handle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // Loop to skip entries we don't like (current and parent directories).
    do
    {
        // Get the next entry if we haven't already.
        if (!m_first_entry)
        {
            // Grab the details of the next entry.
            if (!FindNextFile(m_search_handle, &m_find_data))
            {
                if (GetLastError() != ERROR_NO_MORE_FILES)
                {
                    throw std::string("Error enumerating '") + m_dir + "'.";
                }
                else
                {
                    FindClose(m_search_handle);
                    m_search_handle = INVALID_HANDLE_VALUE;
                }
                return false;
            }
        }
        m_first_entry = false;
    }
    while (strcmp(m_find_data.cFileName, ".") == 0 || strcmp(m_find_data.cFileName, "..") == 0);

    // Return the details.
    name = m_find_data.cFileName;
    type = (m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? file_type::directory : file_type::file;
    return true;
}

#else // Target.

    #error Unsupported target platform.

#endif // Target.


}; //namespace psxdmh
