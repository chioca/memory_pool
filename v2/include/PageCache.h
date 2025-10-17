#pragma once
#include <map>
#include <mutex>

#include "common.h"
namespace memory_pool {
class PageCache {
 public:
  static const size_t PAGE_SIZE = 4096;
  static PageCache& getInstance() {
    static PageCache instance;
    return instance;
  }
  // 分配制定页数的span
  void* allocateSpan(size_t numPages);
  // 释放span
  void deallocateSpan(void* ptr, size_t numPages);

 private:
  /* data */
  PageCache(/* args */) = default;
  void* systemAlloc(size_t numPages);

 private:
  struct Span {
    void* pageAddr;
    size_t numPages;
    Span* next;
  };
  std::map<size_t, Span*> freeSpans_;
  std::map<void*, Span*> spanMap_;
  std::mutex mutex_;
};

}  // namespace memory_pool
