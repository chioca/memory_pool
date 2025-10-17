#include "ThreadCache.h"
namespace memory_pool {
void* ThreadCache::allocate(size_t size) {
  if (size == 0) {
    size = ALIGNMENT;  // 至少分配一个对齐大小
  }
  if (size > MAX_BYTES) {
    return malloc(size);
  }
  size_t index = SizeClass::getIndex(size);
  freeListSize_[index]--;
  void* ptr = freeList_[index];
  // 检查线程本地空闲链表, 若不为空, 则直接分配
  if (ptr != nullptr) {
    freeList_[index] =
        *reinterpret_cast<void**>(ptr);  // 空闲链表指向下一块空闲地址
    return ptr;
  }
  return fetchFromCentralCache(index);
}

void ThreadCache::deallocate(void* ptr, size_t size) {
  if (size > MAX_BYTES) {
    free(ptr);
    return;
  }
  size_t index = SizeClass::getIndex(size);

  // 插入对应空闲链表首位
  *reinterpret_cast<void**>(ptr) = freeList_[index];
  freeList_[index] = ptr;
  // 同时更新空闲链表长度
  freeListSize_[index]++;
  if (shouldReturnToCentralCache(index)) {
    returnToCentralCache(freeList_[index], size);
  }
}

// 判断是否需要将内存回收给中心缓存
bool ThreadCache::shouldReturnToCentralCache(size_t index) {
  return (freeListSize_[index] > THREAD_HOLD);
}
void* ThreadCache::fetchFromCentralCache(size_t size) {}
}  // namespace memory_pool