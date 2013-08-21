set(MSG_DIR ${CMAKE_SOURCE_DIR}/../msg)
set(MSG_LIBRARY_NAME zmsg)

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
#include_directories(${XDB_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
#include_directories(${LIBEXTREMEDB_INCLUDE_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
#link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
#link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

#add_custom_command(
#        OUTPUT 
#        ${XDB_DIR}/dat/xdb_broker.h
#        ${XDB_DIR}/dat/xdb_broker.c
#        ${XDB_DIR}/dat/xdb_broker.hpp
#        COMMAND
#        ${LIBEXTREMEDB_MCOCOMP} -hpp broker.mco -P${XDB_DIR}/dat
#        DEPENDS
#        ${XDB_DIR}/dat/broker.mco
#        WORKING_DIRECTORY
#        ${XDB_DIR}/dat
#        )

set (MSG_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/zmsg.hpp
        ${MSG_DIR}/msg_letter.cpp
        ${MSG_DIR}/msg_letter.hpp)

# Сборка библиотеки
##########################################################
add_library (${MSG_LIBRARY_NAME} ${MSG_SOURCES})
target_link_libraries(${MSG_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

# Сборка брокера
##########################################################
#add_executable (${MDP_BROKER} ${BROKER_SOURCES})
# каталог генерации libmdp.so
#target_link_libraries(${MDP_BROKER}
#    ${LIBZMQ_LIBRARIES}
#    ${TOOLS_LIBRARY_NAME}
#    ${MDP_LIBRARY_NAME}
#    ${XDB_LIBRARY_NAME}
#    ${LIBGLOG_LIBRARIES}
#    ${LIBEXTREMEDB_LIBRARIES_ALL}
#    ${SYSTEM_EXTERNAL_LIBS})

