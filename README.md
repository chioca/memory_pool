# Memory Pool

一个基于 C++ 实现的高性能内存池，用于替代系统默认的 `new/delete`，提升小块内存频繁分配与释放时的性能。

## 特性

- 支持多线程环境（线程缓存 + 中央缓存架构 + 页缓存）  
- 按大小类别管理内存块，减少碎片  
- 简洁接口：`MemoryPool::allocate(size_t)` / `MemoryPool::deallocate(void*, size_t)`  
- 自带单元测试与性能测试（可与系统分配器对比）

## 项目结构
```
├── v1   # 简易版实现
│   ├── CMakeLists.txt
│   ├── include
│   │   └── MemoryPool.h
│   ├── src
│   │   └── MemoryPool.cc
│   └── tests
│       └── UnitTest.cc
└── v2   # 三层缓存内存池 
    ├── CMakeLists.txt
    ├── include
    │   ├── CentralCache.h
    │   ├── common.h
    │   ├── MemoryPool.h
    │   ├── PageCache.h
    │   └── ThreadCache.h
    ├── src
    │   ├── CentralCache.cc
    │   ├── PageCache.cc
    │   └── ThreadCache.cc
    └── tests
        ├── PerformanceTest.cc # 性能测试
        └── UnitTest.cc    # 单元测试
```
