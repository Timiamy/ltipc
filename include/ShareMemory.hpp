#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <Ltlog.hpp>
#include <cstring>
#include <sstream>
#include <string>
#include <defination.hpp>
namespace ltsm {

class ShareMemory {
   public:
    ShareMemory(const std::string& share_name, size_t size, bool is_publisher)
        : share_memory_name_(share_name),
          size_(size + kCPUCacheLineSize),
          is_publisher_(is_publisher),
          share_memory_ptr_(nullptr) 
    {
        int oflag = is_publisher_ ? (O_CREAT | O_RDWR) : O_RDWR;
        int shm_fd = shm_open(share_memory_name_.c_str(), oflag, 0666);
        int rc = ftruncate(shm_fd, static_cast<off_t>(size_));
        if (rc != 0) {
            std::stringstream ss;
            ss << "ftruncate shm fd failed, errno: " << errno;
            logError(ss.str());
        }

        share_memory_ptr_ = mmap(0, size_, PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if(is_publisher)
        {
            std::memset(share_memory_ptr_, 0, size_);
        }
    }

    ShareMemory(const ShareMemory&) = delete;
    ShareMemory(ShareMemory&&) noexcept = delete;
    ShareMemory& operator=(const ShareMemory&) = delete;
    ShareMemory& operator=(ShareMemory&&) noexcept = delete;

    char* GetCharPtr()
    {
        uint64_t ptr = (uint64_t)share_memory_ptr_;
        ptr = (ptr + kCPUCacheLineSize - 1) & ~kCPUCacheLineSize;
        return (char*)ptr;
    }

    ~ShareMemory() {
        if (is_publisher_) {
            munmap(share_memory_ptr_, size_);
            shm_unlink(share_memory_name_.c_str());
        }
    }

   private:
    std::string share_memory_name_;
    size_t size_;
    bool is_publisher_;
    void* share_memory_ptr_;
};
}  // namespace ltsm
