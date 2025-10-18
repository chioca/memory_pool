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
  // 按页数管理空闲span 不同页数对应不同span链表
  std::map<size_t, Span*> freeSpans_;
  // 页号到span的映射，用于回收
  std::map<void*, Span*> spanMap_;
  std::mutex mutex_;
};

}  // namespace memory_pool
