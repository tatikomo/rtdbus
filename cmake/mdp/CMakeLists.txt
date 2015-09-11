set(MDP_BROKER broker)

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
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${XDB_BROKER_DIR})
include_directories(${MSG_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBCPPZMQ_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})

link_directories(${XDB_IMPL_DIR})
link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-Wno-missing-field-initializers)
else (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)

add_custom_command(
        OUTPUT
        ${MSG_DIR}/proto/rtdbus.pb.cc
        ${MSG_DIR}/proto/rtdbus.pb.h
        COMMAND
        sh -c "LD_LIBRARY_PATH=${LIBPROTOBUF_LIBRARIES_DIR} ${PROTOC} --cpp_out=. --proto_path=. rtdbus.proto"
#        ${PROTOC} --cpp_out=. --proto_path=. rtdbus.proto
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
#        ${PROTOC} --cpp_out=. --proto_path=. common.proto
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
#        ${PROTOC} --cpp_out=. --proto_path=. adg.proto
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
#        ${PROTOC} --cpp_out=. --proto_path=. sinf.proto
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
#        ${PROTOC} --cpp_out=. --proto_path=. ech.proto
        DEPENDS
        ${MSG_DIR}/proto/ech.proto
        WORKING_DIRECTORY
        ${MSG_DIR}/proto
        )

add_custom_command(
        OUTPUT 
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp -x broker_db.mco -P${XDB_IMPL_DIR}/dat
        DEPENDS
        ${XDB_IMPL_DIR}/dat/broker_db.mco
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

set (MDP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_client_async_api.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_worker_api.cpp
        ${MSG_DIR}/proto/rtdbus.pb.cc
        ${MSG_DIR}/proto/rtdbus.pb.h
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${MSG_DIR}/proto/adg.pb.cc
        ${MSG_DIR}/proto/adg.pb.h
        ${MSG_DIR}/proto/sinf.pb.cc
        ${MSG_DIR}/proto/sinf.pb.h
        ${MSG_DIR}/proto/ech.pb.cc
        ${MSG_DIR}/proto/ech.pb.h
        ${MSG_DIR}/msg_message.cpp)

set (BROKER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${XDB_IMPL_DIR}/xdb_impl_common.h
        ${XDB_IMPL_DIR}/xdb_impl_common.c
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        ${MDP_DIR}/mdp_proxy.hpp
        ${MDP_DIR}/mdp_proxy.cpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp
        ${MSG_DIR}/msg_message.hpp)

set_source_files_properties(${XDB_IMPL_DIR}/dat/broker_db.h
                            ${XDB_IMPL_DIR}/dat/broker_db.c
                            ${XDB_IMPL_DIR}/dat/broker_db.hpp
                            PROPERTIES GENERATED 1)

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

# Сборка библиотеки
##########################################################
add_library (${MDP_LIBRARY_NAME} ${MDP_SOURCES})
target_link_libraries(${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

# Сборка брокера
##########################################################
add_executable (${MDP_BROKER} ${BROKER_SOURCES})
# каталог генерации libmdp.so
target_link_libraries(${MDP_BROKER}
    ${LIBZMQ_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${XDB_BROKER_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

