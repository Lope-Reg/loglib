# LogLib —— C++ 日志系统

一个教学级日志系统，涵盖 C++ 核心特性：封装、继承、多态、STL、C++11 新特性、智能指针、设计模式。

## 快速开始

```bash
mkdir build && cd build
cmake .. && make -j$(nproc)
./loglib_demo
```

## 在你的项目中使用

**对外只暴露 1 个头文件 + 1 个库**，所有实现细节通过 Pimpl 模式隐藏：

```cpp
#include "loglib/loglib.h"  // 唯一需要的头文件

int main() {
    auto logger = loglib::LoggerManager::getInstance().getRoot();
    LOG_INFO(logger, "Hello LogLib!");
    LOG_INFO_S(logger) << "value=" << 42;
}
```

编译：

```bash
g++ main.cpp -I/path/to/loglib/include -L/path/to/loglib/build -lloglib -lpthread
```

## 安装

```bash
cd build
cmake --install . --prefix /usr/local
# 安装后只有: /usr/local/include/loglib/loglib.h + /usr/local/lib/libloglib.a
```

## 文档

- [设计与实现文档](docs/design.md) —— 架构设计 + 分步实现引导
