#pragma once
#include "common.h"
#define THREAD_HOLD 256
namespace memory_pool {
class ThreadCache {
 private:
  /* data */
  ThreadCache() = default;
  // 从中心缓存获取内存
  void* fetchFromCentralCache(size_t size);
  // 归还内存到中心缓存
  void returnToCentralCache(void* ptr, size_t size);
  // 计算批量获取内存块的数量
  size_t getBatchNum(size_t size);
  // 判断是否需要归还内存
  bool shouldReturnToCentralCache(size_t index);

 private:
  std::array<void*, FREE_LIST_SIZE> freeList_;
  std::array<size_t, FREE_LIST_SIZE> freeListSize_;

 public:
  static ThreadCache* getInstance() {
    static thread_local ThreadCache instance;
    return &instance;
  }
  void* allocate(size_t size);
  void deallocate(void* ptr, size_t size);
};
}  // namespace memory_pool
