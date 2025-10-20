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
  // if (freeListSize_[index] == 0) return fetchFromCentralCache(index);
  // 这里在没有判断分配成功的情况下先自减了数据
  // 如果没有分配成功 会从中心缓存取数据
  // 会返回fetchFromCentralCache 这里会补回这里的一次自减
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
// 计算批量获取内存块的数量
size_t ThreadCache::getBatchNum(size_t size) {
  // 基准：每次批量获取不超过4KB内存
  constexpr size_t MAX_BATCH_SIZE = 4 * 1024;  // 4KB

  // 根据对象大小设置合理的基准批量数
  size_t baseNum;
  if (size <= 32)
    baseNum = 64;  // 64 * 32 = 2KB
  else if (size <= 64)
    baseNum = 32;  // 32 * 64 = 2KB
  else if (size <= 128)
    baseNum = 16;  // 16 * 128 = 2KB
  else if (size <= 256)
    baseNum = 8;  // 8 * 256 = 2KB
  else if (size <= 512)
    baseNum = 4;  // 4 * 512 = 2KB
  else if (size <= 1024)
    baseNum = 2;  // 2 * 1024 = 2KB
  else
    baseNum = 1;  // 大于1024的对象每次只从中心缓存取1个

  // 计算最大批量数
  size_t maxNum = std::max(size_t(1), MAX_BATCH_SIZE / size);

  // 取最小值，但确保至少返回1
  return std::max(sizeof(1), std::min(maxNum, baseNum));
}
}  // namespace memory_pool