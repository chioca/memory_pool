#include "PageCache.h"

#include <sys/mman.h>

#include <cstring>

#include "CentralCache.h"
namespace memory_pool {
void* PageCache::allocateSpan(size_t numPages) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = freeSpans_.lower_bound(numPages);
  // 将取出的span从原有的空闲链表freeSpans_[it->first]中移除

  if (it != freeSpans_.end()) {
    Span* span = it->second;

    if (span->next) {
      freeSpans_[it->first] = span->next;
    } else {
      freeSpans_.erase(it);
    }
    span->next = nullptr;
    // 如果span大于需要的numPages则进行分割
    if (span->numPages > numPages) {
      Span* newSpan = new Span;
      newSpan->pageAddr =
          static_cast<char*>(span->pageAddr) + numPages * PAGE_SIZE;
      newSpan->numPages = span->numPages - numPages;
      newSpan->next = nullptr;

      auto& list = freeSpans_[newSpan->numPages];
      newSpan->next = list;
      list = newSpan;

      span->numPages = numPages;
      span->next = nullptr;
    }
    spanMap_[span->pageAddr] = span;
    return span->pageAddr;
  }

  // 没有合适的span 向系统申请
  void* memory = systemAlloc(numPages);
  if (!memory) return nullptr;

  Span* span = new Span;
  span->pageAddr = memory;
  span->numPages = numPages;
  span->next = nullptr;

  spanMap_[memory] = span;
  return memory;
}
void PageCache::deallocateSpan(void* ptr, size_t numPages) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = spanMap_.find(ptr);
  if (it == spanMap_.end()) return;

  // 查找下一块span
  Span* span = it->second;
  void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
  auto nextIt = spanMap_.find(nextAddr);

  // 当下一块span在空闲span链表中 从中拿出
  if (nextIt != spanMap_.end()) {
    Span* nextSpan = nextIt->second;

    bool found = false;
    auto& nextList = freeSpans_[nextSpan->numPages];

    // 下一块span为链头 直接拿出
    if (nextList == nextSpan) {
      nextList = nextSpan->next;
      found = true;
    } else if (nextList) {
      // 下一块span在链中间
      Span* prev = nextList;
      while (prev->next) {
        if (prev->next == nextSpan) {
          prev->next = nextSpan->next;
          found = true;
          break;
        }
        prev = prev->next;
      }
    }

    // 在空闲链表中找到nextSpan 才对齐进行合并
    if (found) {
      span->numPages += nextSpan->numPages;
      spanMap_.erase(nextAddr);
      delete nextSpan;
    }

    auto& list = freeSpans_[span->numPages];
    span->next = list;
    list = span;
  }
}
void* PageCache::systemAlloc(size_t numPages) {
  size_t size = numPages * PAGE_SIZE;
  void* memory = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (memory == MAP_FAILED) return nullptr;
  memset(memory, 0, size);
  return memory;
}
}  // namespace memory_pool
