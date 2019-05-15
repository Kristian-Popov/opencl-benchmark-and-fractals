#pragma once

#include <boost/compute.hpp>

/*
This class represents a device buffer mapped to host memory
Implements a RAII semantics - buffer is mapped when this
object is created and unmapped when it is destroyed.
Mapping and unmapping is done in asynchronous manner.
*/
template <typename T>
class MappedOpenClBuffer {
public:
    MappedOpenClBuffer(
        boost::compute::vector<T>& vector, boost::compute::command_queue& queue, cl_map_flags flags,
        const boost::compute::wait_list& events)
        : buffer_(vector.get_buffer()), element_count_(vector.size()), queue_(queue) {
        Map(flags, events);
    }

    // Get pointer to host memory.
    boost::compute::future<T*> host_ptr() {
        return boost::compute::future<T*>(host_ptr_, map_event_);
    }

    boost::compute::future<const T*> host_ptr() const {
        return boost::compute::future<T*>(host_ptr_, map_event_);
    }

    ~MappedOpenClBuffer() {
        // TODO it may be useful to allow early unmap to get unmap event, but will have to check
        // to avoid double unmap
        queue_.enqueue_unmap_buffer(buffer_, host_ptr_, boost::compute::wait_list(map_event_));
    }

private:
    void Map(cl_map_flags flags, const boost::compute::wait_list& events) {
        host_ptr_ = static_cast<T*>(queue_.enqueue_map_buffer_async(
            buffer_, flags, 0, element_count_ * sizeof(T), map_event_, events));
    }

    boost::compute::buffer buffer_;  // We must keep a reference to the buffer and queue
    size_t element_count_;
    boost::compute::command_queue queue_;
    boost::compute::event map_event_;
    T* host_ptr_;
};
