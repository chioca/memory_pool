#include "PageCache.h"

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
}  // namespace memory_pool
