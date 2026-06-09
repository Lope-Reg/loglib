cmake_minimum_required(VERSION 3.14)
project(loglib VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 库源文件
set(SOURCES
    src/LogLevel.cpp
    src/PatternFormatter.cpp
    src/LogAppender.cpp
    src/Logger.cpp
    src/LoggerManager.cpp
    src/AsyncAppender.cpp
)

add_library(loglib STATIC ${SOURCES})
target_include_directories(loglib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 示例
add_executable(loglib_demo examples/demo.cpp)
target_link_libraries(loglib_demo loglib pthread)
