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
include_directories(${MSG_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBCPPZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})

link_directories(${LIBPROTOBUF_LIBRARIES_DIR})

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/rtdbus.pb.cc
        ${MSG_DIR}/proto/rtdbus.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. rtdbus.proto"
        DEPENDS
        ${MSG_DIR}/proto/rtdbus.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. common.proto"
        DEPENDS
        ${MSG_DIR}/proto/common.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/adg.pb.cc
        ${MSG_DIR}/proto/adg.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. adg.proto"
        DEPENDS
        ${MSG_DIR}/proto/adg.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/sinf.pb.cc
        ${MSG_DIR}/proto/sinf.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. sinf.proto"
        DEPENDS
        ${MSG_DIR}/proto/sinf.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/ech.pb.cc
        ${MSG_DIR}/proto/ech.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. ech.proto"
        DEPENDS
        ${MSG_DIR}/proto/ech.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

set (MSG_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_zmsg.cpp
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/msg_message.hpp
        ${MSG_DIR}/msg_message.cpp
        ${MSG_DIR}/msg_message_impl.hpp
        ${MSG_DIR}/msg_message_impl.cpp
        ${MSG_DIR}/msg_adm.hpp
        ${MSG_DIR}/msg_adm.cpp
        ${MSG_DIR}/msg_sinf.hpp
        ${MSG_DIR}/msg_sinf.cpp
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${MSG_DIR}/proto/rtdbus.pb.cc
        ${MSG_DIR}/proto/rtdbus.pb.h
        ${MSG_DIR}/proto/adg.pb.cc
        ${MSG_DIR}/proto/adg.pb.h
        ${MSG_DIR}/proto/ech.pb.cc
        ${MSG_DIR}/proto/ech.pb.h
        ${MSG_DIR}/proto/sinf.pb.cc
        ${MSG_DIR}/proto/sinf.pb.h
        )

set_source_files_properties(${MSG_DIR}/proto/rtdbus.pb.cc
                            ${MSG_DIR}/proto/rtdbus.pb.h
                            PROPERTIES GENERATED 1)

set_source_files_properties(${MSG_DIR}/proto/common.pb.cc
                            ${MSG_DIR}/proto/common.pb.h
                            PROPERTIES GENERATED 1)

set_source_files_properties(${MSG_DIR}/proto/sinf.pb.h
                            ${MSG_DIR}/proto/sinf.pb.h
                            PROPERTIES GENERATED 1)

set_source_files_properties(${MSG_DIR}/proto/adg.pb.h
                            ${MSG_DIR}/proto/adg.pb.h
                            PROPERTIES GENERATED 1)

set_source_files_properties(${MSG_DIR}/proto/ech.pb.h
                            ${MSG_DIR}/proto/ech.pb.h
                            PROPERTIES GENERATED 1)

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
  add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-library=stlport4)

# Сборка библиотеки
##########################################################
add_library (${MSG_LIBRARY_NAME} ${MSG_SOURCES})
target_link_libraries(${MSG_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

