set(CLIENT_TRADER trader)
set(CLIENT_PULSAR pulsar)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        rt
        glog
        uuid)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        glog)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${XDB_BROKER_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

set (CLIENT_TRADER_SOURCES
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${CLIENT_DIR}/trader.hpp
        ${CLIENT_DIR}/trader.cpp)

set (CLIENT_PULSAR_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MSG_DIR}/proto/common.pb.h
        ${XDB_BROKER_DIR}/xdb_broker_letter.hpp
        ${CLIENT_DIR}/pulsar.hpp
        ${CLIENT_DIR}/pulsar.cpp)

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

set_source_files_properties(${MSG_DIR}/proto/common.pb.cc
                            ${MSG_DIR}/proto/common.pb.h
                            PROPERTIES GENERATED 1)

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)

# Сборка TRADER
##########################################################
add_executable (${CLIENT_TRADER} ${CLIENT_TRADER_SOURCES})
target_link_libraries(${CLIENT_TRADER}
    ${MDP_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

# Сборка PULSAR
##########################################################
add_executable (${CLIENT_PULSAR} ${CLIENT_PULSAR_SOURCES})
target_link_libraries(${CLIENT_PULSAR}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

