file(GLOB SERVER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB PROTO_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/../*.pb.cc)

add_executable(server ${SERVER_SRCS} ${PROTO_SRCS})
target_link_libraries(server rpc_core)
target_compile_options(server PRIVATE -std=c++17 -Wall)

set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)