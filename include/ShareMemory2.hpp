#pragma once

#include <sys/ipc.h>
#include <sys/shm.h>

#include <ltlog.hpp>
#include <cstring>
#include <defination.hpp>
#include <filesystem>
#include <format>
#include <sstream>
#include <string>
#include <fstream>

namespace ltipc {

class ShareMemory {
   public:
    ShareMemory(const std::string& share_name, size_t size)
        : share_memory_name_(share_name),
          size_(size),
          share_memory_ptr_(nullptr) {
        //std::cout << "share_name: " << share_name << std::endl;

        auto file = std::format("/tmp/{}", share_name);
        auto file_path = std::filesystem::path(file);
        if (!std::filesystem::exists(file_path)) {
            std::ofstream ofs(file_path);
        }
        bool is_creator = false;
        key_t key = ftok(file.c_str(), 0);
        shmid_ = shmget(key, size_, 0666 | IPC_CREAT | IPC_EXCL);
        if (shmid_ == -1) {
            if (errno == EEXIST) {
                shmid_ = shmget(key, size_, 0666 | IPC_CREAT);
            } else {
                LogError("shmget return -1");
            }
        } else {
            is_creator = true;
        }

        share_memory_ptr_ = shmat(shmid_, nullptr, 0);
        if (share_memory_ptr_ == (void*)-1) {
            LogError("shmat return -1");
        } else {
            if (is_creator) {
                std::memset(share_memory_ptr_, 0, size_);
            }
        }

        //std::cout << (is_creator ? "create" : "open")
        //                  << " share_memory_ptr_: " << share_memory_ptr_ << std::endl;
    }

    ShareMemory(const ShareMemory&) = delete;
    ShareMemory(ShareMemory&&) noexcept = delete;
    ShareMemory& operator=(const ShareMemory&) = delete;
    ShareMemory& operator=(ShareMemory&&) noexcept = delete;

    char* GetCharPtr() { return (char*)share_memory_ptr_; }

    ~ShareMemory() { 
        shmdt(share_memory_ptr_);
        shmctl(shmid_, IPC_RMID, NULL);
    }

   private:
    std::string share_memory_name_;
    size_t size_;
    int shmid_;
    void* share_memory_ptr_;
};
}  // namespace ltipc
