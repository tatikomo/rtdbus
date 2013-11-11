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

set (XDB_RTAP_SNAP_NAME "xdb_snap")

include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${XDB_CORE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})

link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBGLOG_LIBRARIES_DIR})

set (XDB_RTAP_SNAP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_RTAP_DIR}/dat/rtap_db.h
        ${XDB_RTAP_DIR}/dat/rtap_db.hpp
        ${XDB_RTAP_DIR}/dat/rtap_db.c
        ${XDB_RTAP_DIR}/xdb_rtap_common.h
        ${XDB_RTAP_DIR}/xdb_rtap_const.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_const.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_main.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_main.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_trans.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_load.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_save.cpp
        )

set (XDB_RTAP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${MDP_DIR}/mdp_letter.hpp
        ${XDB_RTAP_DIR}/dat/rtap_db.h
        ${XDB_RTAP_DIR}/dat/rtap_db.hpp
        ${XDB_RTAP_DIR}/dat/rtap_db.c
        ${XDB_CORE_DIR}/xdb_core_base.cpp
        ${XDB_CORE_DIR}/xdb_core_base.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_const.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_const.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_common.h
        ${XDB_RTAP_DIR}/xdb_rtap_application.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_application.hpp
        ${XDB_CORE_DIR}/xdb_core_base.cpp
        ${XDB_CORE_DIR}/xdb_core_base.hpp
        ${XDB_CORE_DIR}/xdb_core_common.c
        ${XDB_CORE_DIR}/xdb_core_common.h
        ${XDB_RTAP_DIR}/xdb_rtap_database.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_database.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_attribute.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_attribute.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_connection.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_connection.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_error.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_error.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_filter.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_filter.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_message.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_message.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_data.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_data.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_environment.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_environment.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_point.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_point.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_unity.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_unity.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_analog.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_analog.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_binary.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_binary.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_discrete.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_discrete.hpp)

set (XDB_LURKER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_letter.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_lurker.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_lurker.hpp)

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
        ${XDB_RTAP_DIR}/dat/rtap_db.h
        ${XDB_RTAP_DIR}/dat/rtap_db.c
        ${XDB_RTAP_DIR}/dat/rtap_db.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp rtap_db.mco -P${XDB_RTAP_DIR}/dat
        DEPENDS
        ${XDB_RTAP_DIR}/dat/rtap_db.mco
        WORKING_DIRECTORY
        ${XDB_RTAP_DIR}/dat
        )

add_library (${XDB_RTAP_LIBRARY_NAME} ${XDB_RTAP_SOURCES})
target_link_libraries(${XDB_RTAP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${XDB_RTAP_LURKER_NAME} ${XDB_LURKER_SOURCES})
target_link_libraries(${XDB_RTAP_LURKER_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    "-lmcoseri_debug"
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${XDB_RTAP_SNAP_NAME} ${XDB_RTAP_SNAP_SOURCES})
target_link_libraries(${XDB_RTAP_SNAP_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    "-lmcoseri_debug"
    ${SYSTEM_EXTERNAL_LIBS})

