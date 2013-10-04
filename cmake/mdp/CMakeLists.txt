set(MDP_DIR ${CMAKE_SOURCE_DIR}/../mdp)
set(MDP_BROKER broker)
set(MDP_LIBRARY_NAME mdp)

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
include_directories(${XDB_BROKER_DIR})
include_directories(${MSG_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
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

add_custom_command(
        OUTPUT 
        ${XDB_BROKER_DIR}/dat/broker_db.h
        ${XDB_BROKER_DIR}/dat/broker_db.c
        ${XDB_BROKER_DIR}/dat/broker_db.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp broker_db.mco -P${XDB_BROKER_DIR}/dat
        DEPENDS
        ${XDB_BROKER_DIR}/dat/broker_db.mco
        WORKING_DIRECTORY
        ${XDB_BROKER_DIR}/dat
        )

set (MDP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MDP_DIR}/mdp_letter.hpp
        ${MDP_DIR}/mdp_letter.cpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_client_async_api.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_worker_api.cpp)

set (BROKER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_proxy.hpp
        ${MDP_DIR}/mdp_proxy.cpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MSG_DIR}/msg_message.hpp
        ${XDB_BROKER_DIR}/dat/broker_db.h
        ${XDB_BROKER_DIR}/dat/broker_db.c
        ${XDB_BROKER_DIR}/dat/broker_db.hpp
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp)

# Сборка библиотеки
##########################################################
add_library (${MDP_LIBRARY_NAME} ${MDP_SOURCES})
target_link_libraries(${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

# Сборка брокера
##########################################################
add_executable (${MDP_BROKER} ${BROKER_SOURCES})
# каталог генерации libmdp.so
target_link_libraries(${MDP_BROKER}
    ${LIBZMQ_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${XDB_BROKER_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

