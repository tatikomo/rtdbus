set(WORKER_TESTER wtester)
set(WORKER_ECHO   wecho)
set(WORKER_DIGGER wdigger)

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
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${WORKER_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${XDB_CORE_DIR})
include_directories(${TOOLS_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

set (WORKER_TESTER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wtester.hpp
        ${WORKER_DIR}/wtester.cpp)

set (WORKER_ECHO_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wecho.hpp
        ${WORKER_DIR}/wecho.cpp)

set (WORKER_DIGGER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_letter.hpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${XDB_RTAP_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_RTAP_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_RTAP_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_RTAP_DIR}/dat/rtap_db-pimpl.cxx
        ${XDB_RTAP_DIR}/dat/rtap_db.hxx
        ${WORKER_DIR}/wdigger.cpp
        ${WORKER_DIR}/wdigger.hpp)


# Сборка тестового торгового обработчика
##########################################################
add_executable (${WORKER_TESTER} ${WORKER_TESTER_SOURCES})
target_link_libraries(${WORKER_TESTER}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

# Сборка ECHO
##########################################################
add_executable (${WORKER_ECHO} ${WORKER_ECHO_SOURCES})
target_link_libraries(${WORKER_ECHO}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

add_definitions(-fpermissive)
# Сборка DIGGER - сервера БДРВ
##########################################################
add_executable (${WORKER_DIGGER} ${WORKER_DIGGER_SOURCES})
target_link_libraries(${WORKER_DIGGER}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    "-lmcoseri_debug"
    "-lxerces-c"
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

