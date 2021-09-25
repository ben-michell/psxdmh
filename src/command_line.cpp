// psxdmh/src/command_line.cpp
// Command line parsing.
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

#include "command_line.h"
#include "utility.h"


namespace psxdmh
{


//
// Generate a description of the option.
//

std::string
command_line_option::describe() const
{
    std::string str;
    if (m_short_name != 0)
    {
        str += std::string("-") + m_short_name;
        if (!m_arg_name.empty())
        {
            str += std::string(" <") + m_arg_name + ">";
        }
        str += ", ";
    }
    str += std::string("--") + m_long_name;
    if (!m_arg_name.empty())
    {
        str += std::string("=<") + m_arg_name + ">";
    }
    str += "\n";
    if (!m_help.empty())
    {
        str += word_wrap(m_help, 4, 80);
        str += "\n";
    }
    return str;
}


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Command line option definition for bool options.
class bool_option : public command_line_option
{
public:

    // Construction.
    bool_option(std::string long_name, char short_name, bool &value, std::string help) :
        command_line_option(long_name, short_name, "", help),
        m_bool_value(value)
    {
    }

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const
    {
        assert(str == nullptr || *str == 0);
        m_bool_value = true;
    }

private:

    // Target for the option.
    bool &m_bool_value;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Command line option definition for integer-type options.
template<typename T>
class integer_option : public command_line_option
{
public:

    // Construction.
    integer_option(std::string long_name, char short_name, T &value, T min_value, T max_value, std::string arg_name, std::string help) :
        command_line_option(long_name, short_name, arg_name, help),
        m_integer_value(value),
        m_min_value((long) min_value), m_max_value((long) max_value)
    {
        assert(min_value <= max_value);
    }

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const
    {
        m_integer_value = (T) string_to_long(str, m_min_value, m_max_value, option_name);
    }

private:

    // Target for the option.
    T &m_integer_value;

    // Valid range for the option's value.
    long m_min_value;
    long m_max_value;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Command line option definition for double options.
class double_option : public command_line_option
{
public:

    // Construction.
    double_option(std::string long_name, char short_name, double &value, double min_value, double max_value, std::string arg_name, std::string help) :
        command_line_option(long_name, short_name, arg_name, help),
        m_double_value(value),
        m_min_value(min_value), m_max_value(max_value)
    {
        assert(min_value <= max_value);
    }

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const
    {
        m_double_value = string_to_double(str, m_min_value, m_max_value, option_name);
    }

private:

    // Target for the option.
    double &m_double_value;

    // Valid range for the option's value.
    double m_min_value;
    double m_max_value;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Command line option definition for string options.
class string_option : public command_line_option
{
public:

    // Construction.
    string_option(std::string long_name, char short_name, std::string &value, std::string arg_name, std::string help) :
        command_line_option(long_name, short_name, arg_name, help),
        m_string_value(value)
    {
    }

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const
    {
        assert(str != nullptr);
        m_string_value = str;
    }

private:

    // Target for the option.
    std::string &m_string_value;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Command line option definition for callback options.
class callback_option : public command_line_option
{
public:

    // Construction.
    callback_option(std::string long_name, char short_name, command_line::option_callback *callback, std::string arg_name, std::string help) :
        command_line_option(long_name, short_name, arg_name, help),
        m_callback(callback)
    {
    }

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const
    {
        assert(str != nullptr);
        m_callback->call(str);
    }

private:

    // Callback to invoke.
    std::unique_ptr<command_line::option_callback> m_callback;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


//
// Define a bool option.
//

void
command_line::define_bool_option(std::string long_name, char short_name, bool &value, std::string help)
{
    m_options.push_back(std::unique_ptr<command_line_option>(new bool_option(long_name, short_name, value, help)));
}


//
// Define a long option.
//

void
command_line::define_uint_option(std::string long_name, char short_name, uint32_t &value, uint32_t min_value, uint32_t max_value, std::string arg_name, std::string help)
{
    m_options.push_back(std::unique_ptr<command_line_option>(new integer_option<uint32_t>(long_name, short_name, value, min_value, max_value, arg_name, help)));
}


//
// Define a double option.
//

void
command_line::define_double_option(std::string long_name, char short_name, double &value, double min_value, double max_value, std::string arg_name, std::string help)
{
    m_options.push_back(std::unique_ptr<command_line_option>(new double_option(long_name, short_name, value, min_value, max_value, arg_name, help)));
}


//
// Define a string option.
//

void
command_line::define_string_option(std::string long_name, char short_name, std::string &value, std::string arg_name, std::string help)
{
    m_options.push_back(std::unique_ptr<command_line_option>(new string_option(long_name, short_name, value, arg_name, help)));
}


//
// Define a callback option.
//

void
command_line::define_callback_option(std::string long_name, char short_name, option_callback *callback, std::string arg_name, std::string help)
{
    m_options.push_back(std::unique_ptr<command_line_option>(new callback_option(long_name, short_name, callback, arg_name, help)));
}


//
// Parse command line arguments.
//

void
command_line::parse(int argc, char * const argv[], std::vector<std::string> &unhandled) const
{
    // Process the arguments.
    assert(argc >= 0);
    assert(argv != nullptr);
    assert(validate_options());
    bool options_allowed = true;
    for (int arg = 1; arg < argc; ++arg)
    {
        // Store anything that doesn't appear to be an option. Note that a
        // single hyphen by itself is not considered to be an option.
        if (argv[arg][0] != '-' || strcmp(argv[arg], "-") == 0 || !options_allowed)
        {
            unhandled.push_back(argv[arg]);
        }
        // A double hyphen ends option processing.
        else if (strcmp(argv[arg], "--") == 0)
        {
            options_allowed = false;
        }
        // Handle double hyphen options.
        else if (strncmp(argv[arg], "--", 2) == 0)
        {
            // Extract the name and any value specified with it.
            const char *start = &argv[arg][2];
            assert(*start != 0);
            std::string name;
            const char *value = 0;
            const char *equals = strchr(start, '=');
            if (equals == nullptr)
            {
                name = start;
            }
            else
            {
                name.assign(start, equals-start);
                value = equals + 1;
            }

            // Look for a matching option.
            const command_line_option *opt = find_long_option(name);
            if (opt == nullptr)
            {
                throw std::string("Unknown option --") + name + ".";
            }

            // Handle options requiring a value.
            if (opt->has_arg())
            {
                // If no value was specified then use the next argv value.
                if (value == nullptr && ++arg < argc)
                {
                    value = argv[arg];
                }
                if (value == nullptr)
                {
                    throw std::string("Option --") + opt->long_name() + " requires a value.";
                }
                opt->store_value(value, std::string("--") + opt->long_name());
            }
            // Handle valueless options.
            else
            {
                if (value != nullptr)
                {
                    throw std::string("Option --") + opt->long_name() + " does not take a value.";
                }
                opt->store_value("", std::string("--") + opt->long_name());
            }
        }
        // Handle single hyphen options.
        else
        {
            // Process the options.
            assert(argv[arg][0] == '-');
            assert(argv[arg][1] != 0 && argv[arg][1] != '-');
            bool first = true;
            const char *opt_str = &argv[arg][1];
            while (*opt_str != 0)
            {
                // Look for the option.
                char name = *opt_str++;
                const command_line_option *opt = find_short_option(name);
                if (opt == nullptr)
                {
                    throw std::string("Unknown option -") + name + ".";
                }

                // Handle options requring a value.
                if (opt->has_arg())
                {
                    // Only the first option is allowed to require a value.
                    if (!first)
                    {
                        throw std::string("Option -") + opt->short_name() + " requires a value.";
                    }

                    // If no value was specified then use the next argv value.
                    const char *value = *opt_str != 0 ? opt_str : nullptr;
                    opt_str += strlen(opt_str);
                    if (value == nullptr && ++arg < argc)
                    {
                        value = argv[arg];
                    }
                    if (value == nullptr)
                    {
                        throw std::string("Option -") + opt->short_name() + " requires a value.";
                    }
                    opt->store_value(value, std::string("-") + opt->short_name());
                }
                // Handle valueless options.
                else
                {
                    opt->store_value("", std::string("-") + opt->short_name());
                }
                first = false;
            }
        }
    }
}


//
// Generate a description of the command line options.
//

std::string
command_line::describe() const
{
    std::string str;
    for (auto option = m_options.cbegin(); option != m_options.cend(); ++option)
    {
        if (!str.empty())
        {
            str += "\n";
        }
        str += (*option)->describe();
    }
    return str;
}


//
// Find an option by name.
//

const command_line_option *
command_line::find_long_option(std::string name) const
{
    // Search through the options, looking for an exact match or an unambiguous
    // partial match.
    assert(!name.empty());
    bool unique_partial = true;
    const command_line_option *partial = nullptr;
    for (auto option = m_options.cbegin(); option != m_options.cend(); ++option)
    {
        // Check for an exact match.
        if ((*option)->long_name() == name)
        {
            return option->get();
        }

        // Track partial matches.
        if (strncmp((*option)->long_name().c_str(), name.c_str(), name.length()) == 0)
        {
            unique_partial = partial == nullptr;
            partial = option->get();
        }
    }

    // Return a unique partial match, otherwise it wasn't found.
    if (unique_partial && partial != nullptr)
    {
        return partial;
    }
    return nullptr;
}


//
// Find an option by name.
//

const command_line_option *
command_line::find_short_option(char name) const
{
    assert(name != '-');
    assert(name != 0);
    auto predicate = [name](const std::unique_ptr<command_line_option> &opt) { return opt->short_name() == name; };
    auto option = std::find_if(m_options.cbegin(), m_options.cend(), predicate);
    return option != m_options.cend() ? option->get() : nullptr;
}


//
// Validate the options.
//

bool
command_line::validate_options() const
{
    // Ensure both long and short names are unique.
    for (auto opt0 = m_options.cbegin(); opt0 != m_options.cend(); ++opt0)
    {
        for (auto opt1 = opt0 + 1; opt1 != m_options.end(); ++opt1)
        {
            if ((*opt0)->long_name() == (*opt1)->long_name())
            {
                printf("Internal Error: duplicate option long name '%s'\n", (*opt0)->long_name().c_str());
                return false;
            }

            if ((*opt0)->short_name() != 0
                && (*opt1)->short_name() != 0
                && (*opt0)->short_name() == (*opt1)->short_name())
            {
                printf("Internal Error: duplicate option short name '%c'\n", (*opt0)->short_name());
                return false;
            }
        }
    }
    return true;
}


}; //namespace psxdmh
