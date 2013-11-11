if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        pthread
        rt)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  link_directories(/lib/i386-linux-gnu/)
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        pthread
        rt)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${TOOLS_DIR})
include_directories(${XDB_BROKER_DIR})

# Для использования ZMSG и RTDBUS_MessageHeader внутри Letter
include_directories(${MSG_DIR})
include_directories(${MDP_DIR})
include_directories(${XDB_CORE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})

include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})

link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})

set (XDB_BROKER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${XDB_BROKER_DIR}/dat/broker_db.h
        ${XDB_BROKER_DIR}/dat/broker_db.c
        ${XDB_BROKER_DIR}/dat/broker_db.hpp
        ${XDB_CORE_DIR}/xdb_core_common.c
        ${XDB_CORE_DIR}/xdb_core_common.h
        ${XDB_CORE_DIR}/xdb_core_base.cpp
        ${XDB_CORE_DIR}/xdb_core_base.hpp
        ${XDB_BROKER_DIR}/xdb_broker.cpp
        ${XDB_BROKER_DIR}/xdb_broker.hpp
        ${XDB_BROKER_DIR}/xdb_broker_impl.cpp
        ${XDB_BROKER_DIR}/xdb_broker_impl.hpp
        ${XDB_BROKER_DIR}/xdb_broker_service.cpp
        ${XDB_BROKER_DIR}/xdb_broker_service.hpp
        ${XDB_BROKER_DIR}/xdb_broker_worker.cpp
        ${XDB_BROKER_DIR}/xdb_broker_worker.hpp
        ${XDB_BROKER_DIR}/xdb_broker_letter.cpp
        ${XDB_BROKER_DIR}/xdb_broker_letter.hpp
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        )

add_definitions (-Wno-missing-field-initializers)

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

                        
# Сборка библиотеки
##########################################################
#add_definitions("-pthread")
#set (${CMAKE_CXX_FLAGS} "-pthread -lrt" )
#set (${CMAKE_LD_FLAGS} "-pthread -lrt" )
add_library (${XDB_BROKER_LIBRARY_NAME} ${XDB_BROKER_SOURCES})
target_link_libraries(${XDB_BROKER_LIBRARY_NAME}
    ${EXTREMEDB_LIBRARIES_ALL}
    ${MSG_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

