#pragma once
#include <cstddef>
#include <atomic>
#include <memory>
#include <type_traits>

namespace ltipc {
static constexpr size_t kMemoryCacheLineSizeBitLength = 6;
static constexpr size_t kMemoryCacheLineSize = 1 << 6;

template <typename T, typename F>
    requires requires{std::is_standard_layout_v<T> && std::is_trivial_v<T> &&
             std::is_unsigned_v<F>;}
class ElementBlock {
   public:
    static constexpr size_t MemoryCacheLineCount =
        ((sizeof(T) + sizeof(F)) + kMemoryCacheLineSize - 1) >>
        kMemoryCacheLineSizeBitLength;
    static constexpr size_t BlockSize =
        MemoryCacheLineCount * kMemoryCacheLineSize;


    ElementBlock(void* raw_ptr)
        : data_(static_cast<T*>(raw_ptr)),
          flag_(static_cast<std::atomic<F>*>((void*)((char*)raw_ptr + BlockSize - sizeof(F)))) {}

    T* GetDataPtr() { return data_; }

    std::atomic<F>* GetFlagPtr() { return flag_; }

   private:
    T* data_;
    std::atomic<F>* flag_;
};
}  // namespace ltipc