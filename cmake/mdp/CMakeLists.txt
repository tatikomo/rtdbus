set(MDP_DIR ${CMAKE_SOURCE_DIR}/../mdp)
set(MDP_BROKER broker)
set(MDP_LIBRARY_NAME mdp)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
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
include_directories(${XDB_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
#link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

add_custom_command(
        OUTPUT 
        ${XDB_DIR}/dat/xdb_broker.h
        ${XDB_DIR}/dat/xdb_broker.c
        ${XDB_DIR}/dat/xdb_broker.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp broker.mco -P${XDB_DIR}/dat
        DEPENDS
        ${XDB_DIR}/dat/broker.mco
        WORKING_DIRECTORY
        ${XDB_DIR}/dat
        )

set (MDP_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_client_async_api.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_worker_api.cpp)

set (BROKER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/proxy.hpp
        ${MDP_DIR}/proxy.cpp
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${XDB_DIR}/dat/xdb_broker.h
        ${XDB_DIR}/dat/xdb_broker.c
        ${XDB_DIR}/dat/xdb_broker.hpp
        ${MDP_DIR}/mdp_helpers.hpp
        ${MDP_DIR}/mdp_helpers.cpp
#        ${MDP_DIR}/mdp_service.hpp
#        ${MDP_DIR}/mdp_service.cpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp)

# Сборка библиотеки
##########################################################
add_library (${MDP_LIBRARY_NAME} ${MDP_SOURCES})
target_link_libraries(${MDP_LIBRARY_NAME}
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
    ${XDB_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

