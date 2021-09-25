// psxdmh/src/splitter.h
// Split an audio stream into multiple streams.
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

#ifndef PSXDMH_SRC_SPLITTER_H
#define PSXDMH_SRC_SPLITTER_H


#include "module.h"


namespace psxdmh
{


// Split an audio stream into multiple streams.
template <typename S> class splitter : public module<S>
{
public:

    // Construction. This object takes ownership of the source object via the
    // splitter_parent object it creates. Extra splitter objects must be created
    // via the split method. When the last splitter sharing a source destructs
    // it will delete the source. Note that this class communicates with the
    // parent stream, but does not chain to it in the usual module chaining way.
    splitter(module<S> *source) : m_parent(new splitter_parent(source))
    {
        assert(source != nullptr);
        m_parent->attach_child(this);
    }

    // Destruction.
    ~splitter() { m_parent->detach_child(this); }

    // Split off another stream using the same source.
    splitter *split() { return new splitter(m_parent); }

    // Test whether the module is still generating output.
    virtual bool is_running() const { return !m_buffer.empty() || m_parent->is_running(); }

    // Get the next sample.
    virtual bool next(S &s)
    {
        if (m_buffer.empty())
        {
            m_parent->feed_children();
        }
        if (m_buffer.empty())
        {
            s = 0.0;
            return false;
        }
        s = m_buffer.front();
        m_buffer.pop_front();
        return true;
    }

    // Add data to the buffer. Called by the parent.
    void buffer_data(S s) { m_buffer.push_back(s); }

private:

    // Parent for the splitters. This takes care of feeding the source data to
    // all child streams sharing the same source.
    class splitter_parent : uncopyable
    {
    public:

        // Construction.
        splitter_parent(module<S> *source) : m_source(source)
        {
            assert(source != nullptr);
        }

        // Destruction.
        virtual ~splitter_parent() { assert(m_child_streams.empty()); }

        // Test whether the module is still generating output.
        bool is_running() const { return m_source->is_running(); }

        // Load more data into the child stream buffers. This is called by a
        // child when it has exhausted its buffered data and requires more.
        void feed_children()
        {
            S s;
            if (m_source->next(s))
            {
                std::for_each(m_child_streams.begin(), m_child_streams.end(), [&s](splitter *child) { child->buffer_data(s); });
            }
        }

        // Attach a child stream to the parent.
        void attach_child(splitter *child)
        {
            assert(std::find(m_child_streams.begin(), m_child_streams.end(), child) == m_child_streams.end());
            m_child_streams.push_back(child);
        }

        // Detach a child stream from the parent.
        void detach_child(splitter *child)
        {
            auto iter = std::find(m_child_streams.begin(), m_child_streams.end(), child);
            assert(iter != m_child_streams.end());
            m_child_streams.erase(iter);
        }

    private:

        // Source module to be split.
        std::unique_ptr<module<S>> m_source;

        // Currently running child streams.
        std::vector<splitter *> m_child_streams;
    };

    // Construction. This is used for split streams after the initial one to
    // ensure they share ownership of the parent.
    splitter(std::shared_ptr<splitter_parent> parent) : m_parent(parent)
    {
        m_parent->attach_child(this);
    }

    // Parent splitter.
    std::shared_ptr<splitter_parent> m_parent;

    // Buffered data.
    std::deque<S> m_buffer;
};


// Types for mono and stereo splitters.
typedef splitter<mono_t> splitter_mono;
typedef splitter<stereo_t> splitter_stereo;


}; //namespace psxdmh


#endif // PSXDMH_SRC_SPLITTER_H
