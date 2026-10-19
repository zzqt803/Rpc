cmake_minimum_required(VERSION 3.20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
project(SampleRpc CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找vcpkg安装的包
find_package(Protobuf REQUIRED)
find_package(unofficial-zookeeper CONFIG REQUIRED)
find_package(Trantor CONFIG REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(GTest REQUIRED)
find_package(toml11 CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)


add_subdirectory(src)
add_subdirectory(example)