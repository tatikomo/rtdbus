set (PROJECT mdp)
project (${PROJECT})

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

include_directories(${MDP_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})

link_directories(${LIBZMQ_LIBRARIES_DIR})

set (MDP_SOURCES
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_client_async_api.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_worker_api.cpp)

set (BROKER_SOURCES
        ${MDP_DIR}/proxy.hpp
        ${MDP_DIR}/proxy.cpp
        ${MDP_DIR}/zmsg.hpp
        ${MDP_DIR}/zmsg.cpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_service.hpp
        ${MDP_DIR}/mdp_service.cpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp)

# Сборка библиотеки
##########################################################
add_library (${PROJECT} ${MDP_SOURCES})
target_link_libraries(${PROJECT}
    ${LIBZMQ_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

# Сборка брокера
##########################################################
add_executable (${MDP_BROKER} ${BROKER_SOURCES})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)
target_link_libraries(${MDP_BROKER}
    ${LIBZMQ_LIBRARIES}
    ${MDP_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

#add_executable (${CLIENT_NAME} ${CLIENT_SOURCES})
#target_link_libraries(${CLIENT_NAME} 
#    ${LIBZMQ_LIBRARIES} 
#    ${SYSTEM_EXTERNAL_LIBS})
