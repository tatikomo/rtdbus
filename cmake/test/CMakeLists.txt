SET(RTDBUS_DIR ${CMAKE_SOURCE_DIR}/../rtdbus)
SET(TEST_RTDBUS_DIR ${CMAKE_SOURCE_DIR}/../test)

SET(UNITTEST_DEVICE device_unittest)

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
include_directories(${MDP_DIR})
#include_directories(${LIBEV_INCLUDE_DIR})
#include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
#include_directories(${LIBGLOG_INCLUDE_DIR})

link_directories(${CMAKE_SOURCE_DIR}/../../gtest-1.6.0/lib/.libs)
#link_directories(${LIBEV_LIBRARIES_DIR})
#link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
#link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (TEST_DEVICE_SOURCES 
        ${TEST_RTDBUS_DIR}/device_unittest.cpp)

add_executable (${UNITTEST_DEVICE} ${TEST_DEVICE_SOURCES})
target_link_libraries(${UNITTEST_DEVICE}
    ${MDP_LIBRARY_NAME}
    ${LIBZMQ_LIBRARIES}
#    ${LIBGLOG_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

