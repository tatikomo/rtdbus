SET(TEST_RTDBUS_DIR ${CMAKE_SOURCE_DIR}/../test)

SET(UNITTEST_DEVICE device_unittest)
SET(UNITTEST_DATABASE database_unittest)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        rt
#        glog
        gtest
        uuid)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        nsl
#        glog
        gtest)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${CMAKE_SOURCE_DIR}/../../gtest-1.6.0/include)
include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${XDB_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
#include_directories(${LIBGLOG_INCLUDE_DIR})

link_directories(${CMAKE_SOURCE_DIR}/../../gtest-1.6.0/lib/.libs)
link_directories(${LIBRARY_OUTPUT_PATH})
#link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (TEST_DEVICE_SOURCES
        ${XDB_DIR}/dat/xdb_broker.hpp
        ${XDB_DIR}/dat/xdb_broker.h
        ${XDB_DIR}/dat/xdb_broker.c
        ${TEST_RTDBUS_DIR}/device_unittest.cpp)

set (TEST_DATABASE_SOURCES
        ${XDB_DIR}/dat/xdb_broker.hpp
        ${XDB_DIR}/dat/xdb_broker.h
        ${XDB_DIR}/dat/xdb_broker.c
        ${TEST_RTDBUS_DIR}/database_unittest.cpp)

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

add_executable (${UNITTEST_DEVICE} ${TEST_DEVICE_SOURCES})
target_link_libraries(${UNITTEST_DEVICE}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${XDB_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
#    ${LIBGLOG_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DATABASE} ${TEST_DATABASE_SOURCES})
target_link_libraries(${UNITTEST_DATABASE}
    ${XDB_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
#    ${LIBZMQ_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

add_test(NAME XDB_TEST COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DATABASE})
add_test(NAME MDP_TEST COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DEVICE})
