SET(TEST_RTDBUS_DIR ${CMAKE_SOURCE_DIR}/../test)

SET(UNITTEST_DEV_INT  dev_int_test)
SET(UNITTEST_DEV_RUN  dev_run_test)
SET(UNITTEST_DATABASE db_test)
SET(UNITTEST_MESSAGE  msg_test)

SET (GTEST_VERSION "1.7.0")

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        rt
        glog
        gtest
        uuid)
  # XSD Synthethis
  include_directories(/usr/local/include)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        glog
        gtest)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

SET(GTEST_DIR ${CMAKE_SOURCE_DIR}/../../gtest-${GTEST_VERSION})

include_directories(${GTEST_DIR}/include)
include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${XDB_BROKER_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${WORKER_DIR})
include_directories(${CLIENT_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})

link_directories(${GTEST_DIR}/lib/.libs)
link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${MDP_DIR})
link_directories(${XDB_BROKER_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (TEST_DEV_INTERNAL_SOURCES
        ${GTEST_DIR}/include/gtest/gtest.h
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_BROKER_DIR}/xdb_broker_worker.hpp
        ${XDB_BROKER_DIR}/xdb_broker.hpp
        ${XDB_BROKER_DIR}/xdb_broker_service.hpp
        ${XDB_BROKER_DIR}/xdb_broker_letter.hpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_proxy.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${TEST_RTDBUS_DIR}/dev_int_unittest.cpp)

set (TEST_DEV_RUNTIME_SOURCES
        ${GTEST_DIR}/include/gtest/gtest.h
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_BROKER_DIR}/xdb_broker_worker.hpp
        ${XDB_BROKER_DIR}/xdb_broker.hpp
        ${XDB_BROKER_DIR}/xdb_broker_service.hpp
        ${XDB_BROKER_DIR}/xdb_broker_letter.hpp
        ${MDP_DIR}/mdp_broker.hpp
        ${MDP_DIR}/mdp_broker.cpp
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MDP_DIR}/mdp_client_async_api.hpp
        ${MDP_DIR}/mdp_proxy.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${WORKER_DIR}/wdigger.cpp
        ${WORKER_DIR}/wdigger.hpp
        ${CLIENT_DIR}/pulsar.hpp
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${TEST_RTDBUS_DIR}/dev_runt_unittest.cpp)

set (TEST_DATABASE_SOURCES
        ${GTEST_DIR}/include/gtest/gtest.h
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_IMPL_DIR}/xdb_impl_application.hpp
        ${XDB_IMPL_DIR}/xdb_impl_environment.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db.h
        ${XDB_IMPL_DIR}/dat/rtap_db.c
        ${XDB_RTAP_DIR}/xdb_rtap_snap.hpp
        ${XDB_RTAP_DIR}/xdb_rtap_snap_trans.cpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${TEST_RTDBUS_DIR}/database_unittest.cpp)

set (TEST_MSG_SOURCES
        ${GTEST_DIR}/include/gtest/gtest.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_helpers.cpp
        ${MSG_DIR}/proto/common.pb.cc
        ${MSG_DIR}/proto/common.pb.h
        ${TEST_RTDBUS_DIR}/message_unittest.cpp)

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-Wno-missing-field-initializers)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
  add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)

add_custom_command(
        OUTPUT
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        COMMAND
        ${LIBEXTREMEDB_MCOCOMP} -hpp -x broker_db.mco -P${XDB_IMPL_DIR}/dat
        DEPENDS
        ${XDB_IMPL_DIR}/dat/broker_db.mco
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

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
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        COMMAND
        /usr/bin/xsd cxx-parser --type-map rtap_db.map rtap_db.xsd
        DEPENDS
        ${XDB_IMPL_DIR}/dat/rtap_db.xsd
        ${XDB_IMPL_DIR}/dat/rtap_db.map
        WORKING_DIRECTORY
        ${XDB_IMPL_DIR}/dat
        )

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

set_source_files_properties(${XDB_IMPL_DIR}/dat/rtap_db.hpp
                            ${XDB_IMPL_DIR}/dat/rtap_db.h
                            ${XDB_IMPL_DIR}/dat/rtap_db.c
                            PROPERTIES GENERATED 1)

set_source_files_properties(${XDB_IMPL_DIR}/dat/broker_db.h
                            ${XDB_IMPL_DIR}/dat/broker_db.c
                            ${XDB_IMPL_DIR}/dat/broker_db.hpp
                            PROPERTIES GENERATED 1)

set_source_files_properties(${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
                            ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
                            PROPERTIES GENERATED 1)

set_source_files_properties(${MSG_DIR}/proto/common.pb.cc
                            ${MSG_DIR}/proto/common.pb.h
                            PROPERTIES GENERATED 1)

add_executable (${UNITTEST_DEV_INT} ${TEST_DEV_INTERNAL_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
add_definitions(-fpermissive)
target_link_libraries(${UNITTEST_DEV_INT}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_BROKER_INTERFACE_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DEV_RUN} ${TEST_DEV_RUNTIME_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
add_definitions(-fpermissive)
target_link_libraries(${UNITTEST_DEV_RUN}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBGLOG_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DATABASE} ${TEST_DATABASE_SOURCES})
target_link_libraries(${UNITTEST_DATABASE}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_MESSAGE} ${TEST_MSG_SOURCES})
target_link_libraries(${UNITTEST_MESSAGE}
    ${MDP_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${MSG_LIBRARY_NAME}
    ${XDB_BROKER_IMPL_LIBRARY_NAME}
    ${XDB_BROKER_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

add_test(NAME DATABASE_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DATABASE})
add_test(NAME DEV_INT_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DEV_INT})
add_test(NAME DEV_RUN_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DEV_RUN})
add_test(NAME MESSAGE_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_MESSAGE})
