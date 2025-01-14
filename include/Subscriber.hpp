#pragma once
#include <RingBuffer2.hpp>

namespace ltipc {

template <typename T, uint8_t MaxSub, uint CurSub>
class Subscriber {
   private:
    RingBuffer<T, MaxSub> ring_buffer_;

    public:
    Subscriber(): ring_buffer_(CurSub){}

    T Recevie() noexcept
    {
        return ring_buffer_.Read();
    }

    
};
}  // namespace ltipc