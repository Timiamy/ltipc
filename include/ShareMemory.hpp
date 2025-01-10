#pragma once
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ltlog.hpp>
#include <cstring>
#include <defination.hpp>
#include <sstream>
#include <string>

namespace ltipc {

class ShareMemory {
   public:
    ShareMemory(const std::string& share_name, size_t size)
        : share_memory_name_(share_name),
          size_(size),
          share_memory_ptr_(nullptr) {
        //std::cout << "share_name: " << share_name << std::endl;

        int shm_fd = shm_open(share_memory_name_.c_str(),
                              O_CREAT | O_EXCL | O_RDWR, 0666);
        bool is_creator = shm_fd != -1;
        if (is_creator) {
            int rc = ftruncate(shm_fd, static_cast<off_t>(size_));
            if (rc != 0) {
                std::stringstream ss;
                ss << "ftruncate shm fd failed, errno: " << errno;
                LogError(ss.str());
            }
        } else {
            if (errno == EEXIST) {
                shm_fd = shm_open(share_memory_name_.c_str(), O_RDWR, 0666);
                if (shm_fd == -1) {
                    LogError("shm open return -1");
                }
            } else {
                LogError("shm create return -1");
            }
        }

        share_memory_ptr_ =
            mmap(0, size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        //std::cout << (is_creator ? "create": "open") << " share_memory_ptr_: " << share_memory_ptr_ << std::endl;

        if (is_creator) {
            std::memset(share_memory_ptr_, 0, size_);
        }
    }

    ShareMemory(const ShareMemory&) = delete;
    ShareMemory(ShareMemory&&) noexcept = delete;
    ShareMemory& operator=(const ShareMemory&) = delete;
    ShareMemory& operator=(ShareMemory&&) noexcept = delete;

    char* GetCharPtr() {
        return (char*)share_memory_ptr_;
    }

    ~ShareMemory() {
        munmap(share_memory_ptr_, size_);
        shm_unlink(share_memory_name_.c_str());
    }

   private:
    std::string share_memory_name_;
    size_t size_;
    void* share_memory_ptr_;
};
}  // namespace ltipc
