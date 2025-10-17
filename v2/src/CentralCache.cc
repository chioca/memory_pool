#pragma once
#include "CentralCache.h"

#include <chrono>
#include <thread>

#include "PageCache.h"
namespace memory_pool {
const std::chrono::milliseconds CentralCache::DELAY_INTERVAL{1000};

// 每次从PageCache获取Span的Page数量
static const size_t SPAN_PAGES = 8;

CentralCache::CentralCache() {
  for (auto &ptr : centralFreeList_) {
    ptr.store(nullptr, std::memory_order_relaxed);
  }
  for (auto &lock : locks_) {
    lock.clear();
  }
  for (auto &count : delayCounts_) {
    count.store(0, std::memory_order_relaxed);
  }
  for (auto &time : lastReturnTimes_) {
    time = std::chrono::steady_clock::now();
  }
  spanCount_.store(0, std::memory_order_relaxed);
}

void *CentralCache::fetchRange(size_t index) {
  // 索引检查，申请内存过大时应该直接向系统申请
  if (index >= FREE_LIST_SIZE) return nullptr;

  while (locks_[index].test_and_set(std::memory_order_acquire)) {
    std::this_thread::yield();  // 添加线程让步，避免忙等待
  }

  void *result = nullptr;
  try {
    result = centralFreeList_[index].load(std::memory_order_relaxed);
    if (!result) {
      // 中心缓存为空 从页缓存获取新的内存块
      size_t size = (index + 1) * ALIGNMENT;
      result = fetchFromPageCache(size);
      if (!result) {
        locks_[index].clear(std::memory_order_release);
        return nullptr;
      }

      char *start = static_cast<char *>(result);
      size_t numPages =
          (size <= SPAN_PAGES * PageCache::PAGE_SIZE)
              ? SPAN_PAGES
              : (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;
      // 计算实际块数
      size_t blockNum = (numPages * PageCache::PAGE_SIZE) / size;
      if (blockNum > 1) {  // 确保有超过两个块来构造链表
        for (size_t i = 1; i < blockNum; i++) {
          void *current = start + (i - 1) * size;
          void *next = start + i * size;
          *reinterpret_cast<void **>(current) = next;
        }
        *reinterpret_cast<void **>(start + (blockNum - 1) * size) = nullptr;

        void *next = *reinterpret_cast<void **>(result);
        // 将result与链表断开
        *reinterpret_cast<void **>(result) = nullptr;
        centralFreeList_[index].store(next, std::memory_order_release);

        size_t trackrIndex = spanCount_++;
        if (trackrIndex < spanTrackers_.size()) {
          spanTrackers_[trackrIndex].spandAddr.store(start,
                                                     std::memory_order_release);
          spanTrackers_[trackrIndex].numPages.store(numPages,
                                                    std::memory_order_release);
          spanTrackers_[trackrIndex].blockCount.store(
              blockNum, std::memory_order_release);
          spanTrackers_[trackrIndex].freeCount.store(
              blockNum - 1, std::memory_order::memory_order_release);
        }
      }
    } else {
      void *next = *reinterpret_cast<void **>(result);
      *reinterpret_cast<void **>(result) = nullptr;
      centralFreeList_[index].store(next, std::memory_order_release);

      SpanTracker *tracker = getSpanTracker(result);
      if (tracker) {
        tracker->freeCount.fetch_sub(1, std::memory_order_release);
      }
    }
  } catch (...) {
    locks_[index].clear(std::memory_order_release);
    throw;
  }
  // 释放锁
  locks_[index].clear(std::memory_order_release);
  return result;
}

void CentralCache::returnRange(void *start, size_t size, size_t index) {
  if (!start || index >= FREE_LIST_SIZE) {
    return;
  }

  size_t blockSize = (index + 1) * ALIGNMENT;
  size_t blockCount = size / blockSize;
  while (locks_[index].test_and_set(std::memory_order_acquire)) {
    std::this_thread::yield();
  }

  try {
    // 将归还的链表插入中心缓存
    void *end = start;
    size_t count = 1;
    while (*reinterpret_cast<void **>(end) != nullptr && count < blockCount) {
      count++;
      end = *reinterpret_cast<void **>(end);
    }
    void *current = centralFreeList_[index].load(std::memory_order_relaxed);
    *reinterpret_cast<void **>(end) = current;
    centralFreeList_[index].store(start, std::memory_order_release);

    // 更新延迟计数

    size_t currentCount =
        delayCounts_[index].fetch_add(1, std::memory_order_relaxed);
    auto currentTime = std::chrono::steady_clock::now();

    // 检查是否要进行延迟归还
    if (shouldPerformDelayedReturn(index, currentCount, currentTime)) {
      performDelayedReturn(index);
    }

  } catch (...) {
    locks_[index].clear(std::memory_order_release);
    throw;
  }
  locks_[index].clear(std::memory_order_release);
}

}  // namespace memory_pool