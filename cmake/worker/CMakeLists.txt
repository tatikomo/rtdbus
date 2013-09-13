set(WORKER_DIGGER wdigger)
set(WORKER_ECHO   wecho)

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
include_directories(${WORKER_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${TOOLS_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

set (WORKER_DIGGER_SOURCES
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wdigger.hpp
        ${WORKER_DIR}/wdigger.cpp)

set (WORKER_ECHO_SOURCES
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wecho.hpp
        ${WORKER_DIR}/wecho.cpp)


# Сборка DIGGER
##########################################################
add_executable (${WORKER_DIGGER} ${WORKER_DIGGER_SOURCES})
target_link_libraries(${WORKER_DIGGER}
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

