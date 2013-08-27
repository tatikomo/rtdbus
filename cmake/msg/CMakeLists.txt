set(MSG_DIR ${CMAKE_SOURCE_DIR}/../msg)
set(MSG_LIBRARY_NAME zmsg)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        rt
        nsl
        uuid)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        nsl)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        COMMAND
        ${PROTOC} --cpp_out=. --proto_path=. common.proto
        DEPENDS
        ${MSG_DIR}/proto/common.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

set (MSG_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/zmsg.hpp
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/msg_payload.hpp
        ${MSG_DIR}/msg_payload.cpp
        ${MSG_DIR}/msg_message.hpp
        ${MSG_DIR}/msg_message.cpp)

# Сборка библиотеки
##########################################################
add_library (${MSG_LIBRARY_NAME} ${MSG_SOURCES})
target_link_libraries(${MSG_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${LIBPROTOBUF_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

