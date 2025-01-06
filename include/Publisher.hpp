#pragma once
#include <RingBuffer.hpp>

namespace ltsm {

template <typename T, uint8_t MaxSub>
class Publisher {
   private:
    RingBuffer<T, MaxSub> ring_buffer_;

    public:
    Publisher(): ring_buffer_(true, 0){}

    void Send(const T* data)
    {
        ring_buffer_.Write(data);
    }
    
};
}  // namespace ltsm