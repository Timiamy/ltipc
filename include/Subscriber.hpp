#pragma once
#include <RingBuffer.hpp>

namespace ltsm {

template <typename T, uint8_t MaxSub, uint CurSub>
class Subscriber {
   private:
    RingBuffer<T, 2> ring_buffer_;

    public:
    Subscriber(): ring_buffer_(false, CurSub){}

    T Recevie()
    {
        return ring_buffer_.Read();
    }

    
};
}  // namespace ltsm