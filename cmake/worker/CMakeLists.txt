set(WORKER_DIGGER wdigger)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Solaris")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        uuid)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "Solaris")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS "")
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "Solaris")

include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${WORKER_DIR})
include_directories(${MDP_DIR})

link_directories(${LIBZMQ_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

set (WORKER_DIGGER_SOURCES
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wdigger.hpp
        ${WORKER_DIR}/wdigger.cpp)

# Сборка DIGGER
##########################################################
add_executable (${WORKER_DIGGER} ${WORKER_DIGGER_SOURCES})
target_link_libraries(${WORKER_DIGGER}
    ${MDP_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBZMQ_LIBRARIES})

