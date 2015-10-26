if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        pthread
        rt)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
#  link_directories(/lib/i386-linux-gnu/)
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        pthread
        rt)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

set (XDB_RTAP_SNAP_NAME "xdb_snap")

include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_IMPL_DIR}/dat)
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
# Заголовочные файлы нужны при включении xdb_impl_database
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})

link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})

add_custom_command(
        OUTPUT 
        ${XDB_IMPL_DIR}/dat/rtap_db.h
        ${XDB_IMPL_DIR}/dat/rtap_db.c
        ${XDB_IMPL_DIR}/dat/rtap_db.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp -x rtap_db.mco -P${XDB_IMPL_DIR}/dat
        DEPENDS
        ${XDB_IMPL_DIR}/dat/rtap_db.mco
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

# Команда вида:
# xsd cxx-parser --generate-validation --generate-noop-impl rtap_db.xsd
# будет дополнительно генерировать заглушки реализации (rtap_db-pimpl.*)
# перезаписать старые "pimpl.[hc]xx" можно флагом --force-overwrite
# создать тестовый код, использующий сгенерированные классы: --generate-test-drive
add_custom_command(
        OUTPUT
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        COMMAND
        /usr/bin/xsd cxx-parser --type-map rtap_db.map --generate-validation rtap_db.xsd
        DEPENDS
        ${XDB_IMPL_DIR}/dat/rtap_db.xsd
        ${XDB_IMPL_DIR}/dat/rtap_db.map
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

add_custom_command(
        OUTPUT
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.cxx
        COMMAND
        /usr/bin/xsd cxx-parser --type-map rtap_db_dict.map --generate-validation rtap_db_dict.xsd
        DEPENDS
        ${XDB_IMPL_DIR}/dat/rtap_db_dict.xsd
        ${XDB_IMPL_DIR}/dat/rtap_db_dict.map
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

set (XDB_RTAP_SNAP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${TOOLS_DIR}/helper.cpp
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db.h
        ${XDB_IMPL_DIR}/dat/rtap_db.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db.c
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_common.h
        ${XDB_RTAP_DIR}/xdb_rtap_snap.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_main.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_trans.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_load.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_save.cpp
        )

set (XDB_RTAP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${TOOLS_DIR}/helper.cpp
        ${MSG_DIR}/msg_message.hpp
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_common.h
        ${XDB_RTAP_DIR}/xdb_rtap_database.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_database.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_environment.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_environment.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_application.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_application.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_connection.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_connection.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_filter.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_filter.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_message.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_message.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_data.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_data.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_point.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_point.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_unity.cpp
        ${XDB_RTAP_DIR}/xdb_rtap_unity.hpp)

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)

set_source_files_properties(${XDB_IMPL_DIR}/dat/broker_db.h
                            ${XDB_IMPL_DIR}/dat/broker_db.c
                            ${XDB_IMPL_DIR}/dat/broker_db.hpp
                            PROPERTIES GENERATED 1)

add_library (${XDB_RTAP_LIBRARY_NAME} ${XDB_RTAP_SOURCES})
target_link_libraries(${XDB_RTAP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${XDB_RTAP_IMPL_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${XDB_RTAP_SNAP_NAME} ${XDB_RTAP_SNAP_SOURCES})
target_link_libraries(${XDB_RTAP_SNAP_NAME}
    ${LIBGLOG_LIBRARIES}
    ${XDB_RTAP_LIBRARY_NAME}
    ${XDB_RTAP_IMPL_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

