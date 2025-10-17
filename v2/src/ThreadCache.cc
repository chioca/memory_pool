#include "ThreadCache.h"

#include "CentralCache.h"
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
void* ThreadCache::fetchFromCentralCache(size_t index) {
  // 从中心缓存批量获取内存
  void* start = CentralCache::getInstance().fetchRange(index);
  if (!start) return nullptr;

  // 取一个返回, 其余的放回空闲链表
  void* result = start;
  freeList_[index] = *reinterpret_cast<void**>(start);
  size_t batchNum = 0;
  void* current = start;

  while (current != nullptr) {
    batchNum++;
    current = *reinterpret_cast<void**>(current);
  }
  freeListSize_[index] += batchNum;
  return result;
}

void ThreadCache::returnToCentralCache(void* start, size_t size) {
  size_t index = SizeClass::getIndex(size);
  size_t alignedSize = SizeClass::roundUp(size);

  // 计算要归还的内存块数量
  size_t batchNum = freeListSize_[index];
  if (batchNum <= 1) return;

  // 保留一部分在TreadCache中
  size_t keepNum = std::max(batchNum / 4, size_t(1));
  size_t returnNum = batchNum - keepNum;

  char* current = static_cast<char*>(start);
  char* splitNode = current;
  for (size_t i = 0; i < keepNum - 1; i++) {
    splitNode = reinterpret_cast<char*>(*reinterpret_cast<void**>(splitNode));
    if (splitNode == nullptr) {
      returnNum = batchNum - (i + 1);
      break;
    }
  }
  if (splitNode != nullptr) {
    // 断开要返回的部分和要保留的部分
    void* nextNode = *reinterpret_cast<void**>(splitNode);
    *reinterpret_cast<void**>(splitNode) = nullptr;

    // 更新ThreadCache的空闲链表
    // 感觉这里不必要更新,freeList[index] 并没后改变
    freeList_[index] = start;

    // 更新自由链表大小
    freeListSize_[index] = keepNum;
    if (returnNum > 0 && nextNode != nullptr) {
      CentralCache::getInstance().returnRange(nextNode, returnNum * alignedSize,
                                              index);
    }
  }
}
}  // namespace memory_pool