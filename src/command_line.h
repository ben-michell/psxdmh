// psxdmh/src/command_line.h
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

#ifndef PSXDMH_SRC_COMMAND_LINE_H
#define PSXDMH_SRC_COMMAND_LINE_H


#include "message.h"
#include "utility.h"


namespace psxdmh
{


// Forwards.
class command_line_option;


// Collection of command line options and their parser. The syntax of the
// command line is as follows:
// - Short names for options that do not require a value can be grouped together
//   in a single argv value. Example: "-abc" sets the 'a', 'b', and 'c' options.
// - Short names for an options requiring a value can either append it to the
//   same argv value, or can supply it in the following argv value. Example:
//   "-d0" and "-d 0" both set the 'd' option to '0'.
// - A single hyphen by itself is not considered to be an option, and is treated
//   as an argument.
// - Long names for an option can be specified with either their exact name or
//   enough of their name to uniquely identify the option. If the option
//   requires a value it can be specified either as an equals sign followed by
//   the value in the same argv value, or in the next argv value. Examples:
//   "--foo" sets the 'foo' option; "--bar=baz" and "--bar baz" both set the
//   'bar' option to 'baz'.
// - When '--' is specified by itself then no futher option processing is done.
class command_line : public uncopyable
{
public:

    // Base class for option callbacks.
    class option_callback : public uncopyable
    {
    public:

        option_callback() {}
        virtual ~option_callback() {}
        virtual void call(std::string value) = 0;
    };

    // Enum option callback.
    template <typename E> class enum_callback : public option_callback
    {
    public:

        enum_callback(E &target, E value) : m_target(target), m_value(value) {}
        virtual void call(std::string) { m_target = m_value; }

    private:

        E &m_target;
        E m_value;
    };

    // Custom string option callback.
    template <typename T> class custom_string_callback : public option_callback
    {
    public:

        typedef void (T::*method_signature)(std::string);

        custom_string_callback(T &target, method_signature method) : m_target(target), m_method(method) {}
        virtual void call(std::string value) { (m_target.*m_method)(value); }

    private:

        T &m_target;
        method_signature m_method;
    };

    // Message verbosity option callback.
    class verbosity_callback : public option_callback
    {
    public:

        verbosity_callback(verbosity value) : m_value(value) {}
        virtual void call(std::string) { message::verbosity(m_value); }

    private:

        verbosity m_value;
    };

    // Construction.
    command_line() {}

    // Define options. The short_name is optional and may be set to 0 if not
    // required. All other fields are required. The bool option is set to true
    // when found. The uint32_t, double, and string options are set to the value
    // provided on the command line. The callback option invokes the given
    // callback. The enum option sets the enum to the provided value. The
    // verbosity option set the message verbosity.
    void define_bool_option(std::string long_name, char short_name, bool &value, std::string help);
    void define_uint_option(std::string long_name, char short_name, uint32_t &value, uint32_t min_value, uint32_t max_value, std::string arg_name, std::string help);
    void define_double_option(std::string long_name, char short_name, double &value, double min_value, double max_value, std::string arg_name, std::string help);
    void define_string_option(std::string long_name, char short_name, std::string &value, std::string arg_name, std::string help);
    void define_callback_option(std::string long_name, char short_name, option_callback *callback, std::string arg_name, std::string help);
    template<typename E> void define_enum_option(std::string long_name, char short_name, E &value, E enum_value, std::string help) { define_callback_option(long_name, short_name, new enum_callback<E>(value, enum_value), "", help); }
    void define_verbosity_option(std::string long_name, char short_name, verbosity value, std::string help) { define_callback_option(long_name, short_name, new verbosity_callback(value), "", help); }

    // Parse command line arguments. Any arguments not processed are returned
    // via unhandled. Errors are reported via a thrown std::string.
    void parse(int argc, char * const argv[], std::vector<std::string> &unhandled) const;

    // Generate a description of the command line options.
    std::string describe() const;

private:

    // Find an option by name.
    const command_line_option *find_long_option(std::string name) const;
    const command_line_option *find_short_option(char name) const;

    // Validate the options. Returns true if valid, false if not.
    bool validate_options() const;

    // Set of options.
    std::vector<std::unique_ptr<command_line_option>> m_options;
};


//  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// Base class for command line option definition classes.
class command_line_option
{
public:

    // Construction. The short_name is optional and may be set to 0 if not
    // required. The arg_name may be blank if no argument is required. All other
    // fields are required.
    command_line_option(std::string long_name, char short_name, std::string arg_name, std::string help) :
        m_long_name(long_name), m_short_name(short_name),
        m_arg_name(arg_name),
        m_help(help)
    {
        assert(long_name.length() > 0 && long_name[0] != '-');
        assert(long_name.find('=') == long_name.npos);
        assert(short_name != '-');
    }

    // Virtualize the destructor.
    virtual ~command_line_option() {}

    // Long and short option names.
    std::string long_name() const { return m_long_name; }
    char short_name() const { return m_short_name; }

    // Whether the option has an argument.
    bool has_arg() const { return !m_arg_name.empty(); }

    // Generate a description of the option.
    std::string describe() const;

    // Parse and store the value. Errors are reported via a thrown std::string.
    virtual void store_value(const char *str, std::string option_name) const = 0;

private:

    // Attributes of the option.
    std::string m_long_name;
    char m_short_name;
    std::string m_arg_name;
    std::string m_help;
};


}; // namespace psxdmh


#endif // PSXDMH_SRC_COMMAND_LINE_H
