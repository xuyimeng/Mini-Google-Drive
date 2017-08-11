project(grpc-cmake-example)
cmake_minimum_required(VERSION 3.2)

add_compile_options(-std=c++11)

# GRPC and Protocol Buffers libraries location
list(APPEND CMAKE_PREFIX_PATH "/opt/grpc" "/opt/protobuf")

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(GRPC REQUIRED)
find_package(BIND REQUIRED)
include_directories(${BIND_SRC_DIR})


set(CMAKE_AUTOMOC ON)
set(CMAKE_PREFIX_PATH /Users/yimengxu/Qt/5.8/clang_64/lib/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
find_package(Qt5Core)
find_package(Qt5Network)


set(PROTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/protos/bigtable.proto
)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})

# Building server
# master node
add_executable(master_server
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/master_server.cc

    ${PROTO_SRCS}
    ${GRPC_SRCS}
)
target_link_libraries(master_server
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

# worker node
add_executable(bigtable_server
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/bigtable_server.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/timer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/bigtable.cc

    ${PROTO_SRCS}
    ${GRPC_SRCS}
)
target_link_libraries(bigtable_server
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

# Building client
add_executable(bigtable_client
    ${CMAKE_CURRENT_SOURCE_DIR}/backend/bigtable_client.cc
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)
target_link_libraries(bigtable_client
    gRPC::grpc++_reflection
    protobuf::libprotobuf
)

# building http
add_executable(httpServer 

    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/httpclient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/httpcookie.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/httpsession.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/httprequest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/httpresponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/loginservlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/emailservlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/storageservlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/fileioservlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/consoleservlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/servlet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/servletmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/tcpserver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpserver/tcpworker.cpp
    
	${PROTO_SRCS}
	${GRPC_SRCS}
    ${BIND_SRCS}
)

# Add the Qt5 Widgets for linking
target_link_libraries(httpServer
				gRPC::grpc++_reflection
    			protobuf::libprotobuf
    			 Qt5::Core Qt5::Network
                 ${BIND_LIBRARY})
