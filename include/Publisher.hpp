#pragma once
#include <RingBuffer2.hpp>

namespace ltipc {

template <typename T, uint8_t MaxSub>
class Publisher {
   private:
    RingBuffer<T, MaxSub> ring_buffer_;

    public:
    Publisher(): ring_buffer_({}){}

    void Send(const T& data) noexcept
    {
        ring_buffer_.Write(data);
    }
    
};
}  // namespace ltipc