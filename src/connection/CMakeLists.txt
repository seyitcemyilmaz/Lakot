cmake_minimum_required(VERSION 3.16)

project(LakotConnection LANGUAGES CXX)

find_package(Protobuf CONFIG REQUIRED)

set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/proto)

set(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/common.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/protocol.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/connection.proto

    ${CMAKE_CURRENT_SOURCE_DIR}/proto/services/auth.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/services/chat.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/services/shop.proto
)

protobuf_generate(
    LANGUAGE cpp
    OUT_VAR PROTO_SRCS
    PROTOS ${PROTO_FILES}
    PROTOC_OUT_DIR ${PROTO_OUT_DIR}
    IMPORT_DIRS ${PROTO_ROOT}
)

add_library(${PROJECT_NAME} STATIC
    connection_global.h

    ${PROTO_SRCS}

    NetworkSession.h
    NetworkClient.h

    SessionRegistry.h
    MessageDispatcher.h
    NetworkServer.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

### add boost - v1.87.0 - begin ###
set(Boost_DIR ${LAKOT_EXTERNAL_PATH}/boost/windows/lib/cmake/Boost-1.87.0)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.87 CONFIG REQUIRED COMPONENTS asio)
target_link_libraries(${PROJECT_NAME} PUBLIC Boost::asio)
### add boost - v1.87.0 - end ###

target_include_directories(${PROJECT_NAME} PUBLIC ${PROTO_OUT_DIR})
target_sources(${PROJECT_NAME} PUBLIC ${PROTO_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC
    LakotUtils
    protobuf::libprotobuf
)

target_compile_definitions(${PROJECT_NAME} PUBLIC LAKOT_CONNECTION_LIBRARY)
