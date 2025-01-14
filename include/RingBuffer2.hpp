

#pragma once
#include <ElementBlock.hpp>
#include <ShareMemory2.hpp>
#include <atomic>
#include <cstddef>
#include <ltlog.hpp>

namespace ltipc {
template <size_t SubscriberNumber>
constexpr auto GetFlagType() {
    if constexpr (SubscriberNumber < 64 && SubscriberNumber > 32) {
        return uint64_t{};
    } else if constexpr (SubscriberNumber < 32 && SubscriberNumber > 16) {
        return uint32_t{};
    } else if constexpr (SubscriberNumber < 16 && SubscriberNumber > 8) {
        return uint16_t{};
    } else {
        return uint8_t{};
    }
}

template <typename T, size_t SubscriberNumber, size_t ElementNumber = 256,
          bool Blocking = false, bool FullWarning = true>
    requires requires { ElementNumber&(ElementNumber - 1) == 0; }
class RingBuffer {
    static_assert(SubscriberNumber < 64 && SubscriberNumber > 0,
                  "SubscriberNumber must gt 0 and lt 64");

    using FlagType = decltype(GetFlagType<SubscriberNumber>());
    using ElementBlockT = ElementBlock<T, FlagType>;
    static constexpr size_t ElementSize = ElementBlockT::BlockSize;
    static constexpr size_t ShareMemorySize = ElementNumber * ElementSize;
    static constexpr size_t FlagMask = (1 << (SubscriberNumber + 1)) - 1;

   public:
    explicit RingBuffer(int member_index = 0)
        : member_index_(member_index),
          element_index_(0),
          share_memory_(std::make_unique<ShareMemory>(typeid(*this).name(),
                                                      ShareMemorySize)) {
        auto* raw_ptr = share_memory_->GetCharPtr();
        for (size_t i = 0; i < elements_.size(); i++) {
            elements_[i] =
                std::make_unique<ElementBlockT>(raw_ptr + i * ElementSize);
        }
    }

    T Read() noexcept {
        auto pos = GetPostion();

        std::unique_ptr<ElementBlockT>& element = elements_.at(pos);

        std::atomic<FlagType>* flag = element->GetFlagPtr();

        while (flag->fetch_and(1, std::memory_order_acquire) == 0);

        T data = *element->GetDataPtr();

        flag->fetch_or(1 << member_index_, std::memory_order_acquire);

        FlagType flg = flag->load(std::memory_order_relaxed);

        if (flg == FlagMask) {
            flag->store(0, std::memory_order_release);
        }

        element_index_++;

        return data;
    }

    void Write(const T& data) noexcept {
        auto pos = GetPostion();

        std::unique_ptr<ElementBlockT>& element = elements_.at(pos);
        std::atomic<FlagType>* flag = element->GetFlagPtr();

        if constexpr (Blocking) {
            while (flag->fetch_and(1, std::memory_order_acquire) == 1);
        } else {
            if constexpr (FullWarning) {
                if (flag->fetch_and(1, std::memory_order_acquire) == 1) {
                    LogWarning(
                        std::format("Subscriber didn't get the info, flag: {}",
                                    flag->load(std::memory_order_relaxed)));
                }
            }
        }

        auto* data_ptr = element->GetDataPtr();
        std::memcpy(data_ptr, &data, sizeof(T));
        flag->store(1, std::memory_order_release);

        element_index_++;
    }

   private:
    size_t GetPostion() { return element_index_ & (ElementNumber - 1); }

    size_t GetNextPostion() {
        return (element_index_ + 1) & (ElementNumber - 1);
    }

    uint8_t member_index_;  // 0 for publisher, others for sub
    size_t element_index_;
    std::unique_ptr<ShareMemory> share_memory_;
    std::array<std::unique_ptr<ElementBlockT>, ElementNumber> elements_;
};
}  // namespace ltipc