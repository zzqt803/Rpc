#获取当前目录下的所有源文件
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 获取 protobuf 的生成文件
file(GLOB PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.pb.cc)

#创建静态库或共享库
add_library(rpc_core STATIC ${SRC_FILES} ${PROTO_SRCS})

# 开启zookeeper的同步API
target_compile_definitions(rpc_core PRIVATE THREADED)
# 链接所有库
target_link_libraries(rpc_core PUBLIC
    protobuf::libprotobuf
    unofficial::zookeeper::zookeeper
    Trantor::Trantor
    fmt::fmt
    spdlog::spdlog
    toml11::toml11
    CLI11::CLI11
)

#设置头文件的路径
target_include_directories(rpc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 添加编译选项
target_compile_options(rpc_core PRIVATE -std=c++17 -Wall)