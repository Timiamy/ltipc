#pragma once
#include <ShareMemory.hpp>
#include <bitset>
#include <defination.hpp>
#include <memory>
#include <optional>
#include <atomic>
#include <sstream>
#include <typeinfo>
#include <type_traits>
#include <thread>

namespace ltsm {
template <typename T, size_t SubscriberNumber, size_t ElementNumber = 64>
class RingBuffer {
    static constexpr size_t ElementCacheLine =
        (sizeof(T) + kCPUCacheLineSize - 1) / kCPUCacheLineSize;
    static constexpr size_t StatusBitsetCacheLine =
        ((ElementNumber >> 3) + kCPUCacheLineSize - 1) / kCPUCacheLineSize;

    static constexpr size_t ShareMemorySize =
        (ElementCacheLine * ElementNumber +
         StatusBitsetCacheLine * (1 + SubscriberNumber)) *
        kCPUCacheLineSize;

    static constexpr size_t ElementOffset = 0;
    static constexpr size_t PublisherStatusBitSetOffset =
        ElementCacheLine * ElementNumber * kCPUCacheLineSize;
    static constexpr size_t SubscriberStatusBitSetOffset =
        PublisherStatusBitSetOffset + StatusBitsetCacheLine * kCPUCacheLineSize;

    template <size_t SZ>
    static constexpr auto GetMaxLockFreeSize() {
        static_assert((SZ & (SZ - 1)) == 0);
        if constexpr (std::atomic<std::bitset<SZ * 8>>::is_always_lock_free) {
            return SZ;
        }
        return GetMaxLockFreeSize<SZ / 2>();
    }

    static constexpr size_t MaxLockFreeSize = GetMaxLockFreeSize<1024>();

   private:
    bool is_piblisher_;
    uint8_t subscriber_id_;
    size_t position_;
    std::unique_ptr<ShareMemory> share_memory_;
    char* publish_status_ptr_;
    char* subscriber_status_ptr_;

   public:
    RingBuffer(bool is_piblisher, uint8_t subscriber_id = 0):
        is_piblisher_(is_piblisher), subscriber_id_(subscriber_id),position_(0),
         share_memory_(std::make_unique<ShareMemory>(typeid(*this).name(),
                                                      ShareMemorySize,
                                                      is_piblisher)) {
        static_assert(ElementNumber % 64 == 0,
                      "Element number need divisible by 64");
        static_assert(std::is_trivially_copyable_v<T>,"T must trivially copyable");

        auto* raw_ptr = share_memory_->GetCharPtr();

        publish_status_ptr_ = raw_ptr + PublisherStatusBitSetOffset;
        subscriber_status_ptr_ = raw_ptr + SubscriberStatusBitSetOffset;
    }

    T Read() {

        ClearSubscriberNextElement();

        auto pos = GetPostion();
        auto [pub_bitset_ptr, pub_index] = GetBitSetOffset(pos, publish_status_ptr_);
        while(!pub_bitset_ptr->load(std::memory_order_acquire).test(pub_index))
        {
            std::this_thread::yield();
        }

        auto* raw_ptr = share_memory_->GetCharPtr();
        auto* data_ptr = raw_ptr + ElementCacheLine * GetPostion() * kCPUCacheLineSize;
        T value{};
        std::memcpy(&value, data_ptr, sizeof(T));

        SetSubscriberCurrentElement();
        ++ position_;

        return value;
    }

    void Write(const T* data) { 
        ClearPublisherNextElement(); 

        auto* raw_ptr = share_memory_->GetCharPtr();
        auto* data_ptr = raw_ptr + ElementCacheLine * GetPostion() * kCPUCacheLineSize;
        std::memcpy(data_ptr, data, sizeof(T));

        SetPublisherCurrentElement();
        ++ position_;
    }


   private:

    void ClearSubscriberNextElement() {
        auto next_pos = GetNextPostion();
        auto subscriber_status_ptr = GetSubScriberPtr();
        auto [sub_bitset_ptr, sub_index] = GetBitSetOffset(next_pos, subscriber_status_ptr);
        auto sub_bitset = sub_bitset_ptr->load(std::memory_order_acquire);
        sub_bitset.reset(sub_index);
        sub_bitset_ptr->store(sub_index, std::memory_order_release);
    }

    void SetSubscriberCurrentElement()
    {
        auto pos = GetPostion();
        auto subscriber_status_ptr = GetSubScriberPtr();

        auto [sub_bitset_ptr, sub_index] = GetBitSetOffset(pos, subscriber_status_ptr);
        auto sub_bitset = sub_bitset_ptr->load(std::memory_order_acquire);
        sub_bitset.set(sub_index);
        sub_bitset_ptr->store(sub_index, std::memory_order_release);
    }


    void ClearPublisherNextElement() {
        auto next_pos = GetNextPostion();
        auto [pub_bitset_ptr, pub_index] = GetBitSetOffset(next_pos, publish_status_ptr_);
        auto pub_bitset = pub_bitset_ptr->load(std::memory_order_acquire);
        if( pub_bitset.test(pub_index))
        {
            for (size_t i = 0; i < SubscriberNumber; i++)
            {
                auto sub_base_ptr = subscriber_status_ptr_ + StatusBitsetCacheLine * kCPUCacheLineSize;
                auto [sub_bitset_ptr, sub_index] = GetBitSetOffset(next_pos, sub_base_ptr);

                if(!sub_bitset_ptr->load(std::memory_order_acquire).test(sub_index))
                {
                    std::stringstream ss;
                    ss << "subscriber " << i << " didn't process the message number " << position_ + 1 << " ,index "  << next_pos;
                    logError(ss.str());
                }
            }
            pub_bitset.reset(pub_index);
            pub_bitset_ptr->store(pub_bitset, std::memory_order_release);
        }
    }

    void SetPublisherCurrentElement()
    {
        auto pos = GetPostion();

        auto [pub_bitset_ptr, pub_index] = GetBitSetOffset(pos, publish_status_ptr_);
        auto pub_bitset = pub_bitset_ptr->load(std::memory_order_acquire);
        if( !pub_bitset.test(pub_index))
        {
            pub_bitset.set(pub_index);
            pub_bitset_ptr->store(pub_bitset, std::memory_order_release);
        }else
        {
            std::stringstream ss;
            ss << "publisher didn't process the message number " << position_ << " ,index "  << pos;
            logError(ss.str());
        }
    }

    std::tuple<std::atomic<std::bitset<MaxLockFreeSize>>*, size_t>
    GetBitSetOffset(size_t position, char* base_ptr) {
        size_t offset = position / MaxLockFreeSize / 8;
        size_t index = position - offset * MaxLockFreeSize * 8;

        std::atomic<std::bitset<MaxLockFreeSize>>* bitset_ptr =
            reinterpret_cast<std::atomic<std::bitset<MaxLockFreeSize>>*>(
                base_ptr + offset * MaxLockFreeSize);
        
        return {bitset_ptr, index};
    }

    char* GetSubScriberPtr()
    {
        return  subscriber_status_ptr_ + StatusBitsetCacheLine * kCPUCacheLineSize * subscriber_id_;
    }

    size_t GetPostion() { return position_ & (ElementNumber - 1); }

    size_t GetNextPostion() { return (position_ + 1) & (ElementNumber - 1); }
};
};  // namespace ltsm