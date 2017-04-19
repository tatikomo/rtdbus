SET(TEST_RTDBUS_DIR ${CMAKE_SOURCE_DIR}/../test)

SET(UNITTEST_DEV_INT  dev_int_test)
SET(UNITTEST_DEV_RUN  dev_run_test)
SET(UNITTEST_DATABASE db_test)
SET(UNITTEST_MESSAGE  msg_test)
SET(UNITTEST_METRICS  metric_test)
SET(UNITTEST_EXCHANGE exch_test)
SET(UNITTEST_CE       ce_test)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        rt
        pthread
        uuid)
  # XSD Synthethis
  include_directories(/usr/local/include)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        pthread)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

SET(GTEST_DIR ${CMAKE_SOURCE_DIR}/../../googletest/googletest)
SET(GTEST_INC_DIR ${GTEST_DIR}/include)

include_directories(${GTEST_INC_DIR})
include_directories(${TOOLS_DIR})
include_directories(${MDP_DIR})
include_directories(${HDB_DIR})
include_directories(${MSG_DIR})
include_directories(${RAPIDJSON_INCLUDE_DIR})
include_directories(${EXCHANGE_EGSA_DIR})
include_directories(${EXCHANGE_CONFIG_DIR})
include_directories(${EXCHANGE_SA_DIR})
include_directories(${EXCHANGE_SMAD_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_BROKER_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${WORKER_DIR})
include_directories(${CLIENT_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBCPPZMQ_INCLUDE_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBSQLITE_INCLUDE_DIR})
include_directories(${XERCESC_INCLUDE_DIR})


link_directories(${GTEST_DIR}/../googlemock/gtest)
link_directories(${LIBGLOG_LIBRARIES_DIR})
#link_directories(${MDP_DIR})
#link_directories(${HDB_DIR})
link_directories(${XERCESC_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBSQLITE_LIBRARIES_DIR})

set (TEST_EXCHANGE_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request.cpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_int.hpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_ext.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_site.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_site.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config_egsa.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config_egsa.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config_sac.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config.hpp
        ${TEST_RTDBUS_DIR}/exchange_unittest.cpp)

set (TEST_METRICS_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
        ${TOOLS_DIR}/tool_metrics.hpp
        ${TEST_RTDBUS_DIR}/metrics_unittest.cpp)

set (TEST_DEV_INTERNAL_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
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
        ${MSG_DIR}/msg_message.hpp
        ${MSG_DIR}/msg_adm.hpp
        ${MSG_DIR}/msg_sinf.hpp
        ${MSG_DIR}/proto/common.proto
        ${MSG_DIR}/proto/rtdbus.proto
        ${MSG_DIR}/proto/sinf.proto
        ${MSG_DIR}/proto/adg.proto
        ${MSG_DIR}/proto/ech.proto
        ${TEST_RTDBUS_DIR}/dev_int_unittest.cpp)

set (TEST_DEV_RUNTIME_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
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
        ${MSG_DIR}/proto/common.proto
        ${MSG_DIR}/proto/rtdbus.proto
        ${MSG_DIR}/proto/sinf.proto
        ${MSG_DIR}/proto/adg.proto
        ${MSG_DIR}/proto/ech.proto
        ${TEST_RTDBUS_DIR}/dev_runt_unittest.cpp)

set (TEST_DATABASE_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
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
        ${HDB_DIR}/hdb_impl_processor.hpp
        ${TEST_RTDBUS_DIR}/database_unittest.cpp)

set (TEST_MSG_SOURCES
        ${GTEST_INC_DIR}/gtest/gtest.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_helpers.cpp
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
        ${MSG_DIR}/proto/common.proto
        ${MSG_DIR}/proto/rtdbus.proto
        ${MSG_DIR}/proto/sinf.proto
        ${MSG_DIR}/proto/adg.proto
        ${MSG_DIR}/proto/ech.proto
        ${MSG_DIR}/msg_common.h
        ${MSG_DIR}/msg_message.hpp
        ${MSG_DIR}/msg_adm.hpp
        ${MSG_DIR}/msg_sinf.hpp
        ${TEST_RTDBUS_DIR}/message_unittest.cpp)

set (TEST_CE_SOURCES
        ${TEST_RTDBUS_DIR}/ce_unittest.hpp
        ${TEST_RTDBUS_DIR}/ce_unittest.cpp)

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

add_executable (${UNITTEST_EXCHANGE} ${TEST_EXCHANGE_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
add_definitions (-pg)
target_link_libraries(${UNITTEST_EXCHANGE}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${EXCHANGE_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_METRICS} ${TEST_METRICS_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
target_link_libraries(${UNITTEST_METRICS}
    ${TOOLS_LIBRARY_NAME}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DEV_INT} ${TEST_DEV_INTERNAL_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
#add_definitions(-fpermissive)
target_link_libraries(${UNITTEST_DEV_INT}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_BROKER_INTERFACE_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DEV_RUN} ${TEST_DEV_RUNTIME_SOURCES})
add_definitions (-D_FUNCTIONAL_TEST)
#add_definitions(-fpermissive)
target_link_libraries(${UNITTEST_DEV_RUN}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_DATABASE} ${TEST_DATABASE_SOURCES})
target_link_libraries(${UNITTEST_DATABASE}
    ${XDB_BROKER_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${HDB_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${LIBGLOG_LIBRARIES}
    ${LIBSQLITE_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_MESSAGE} ${TEST_MSG_SOURCES})
target_link_libraries(${UNITTEST_MESSAGE}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${XDB_BROKER_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBPROTOBUF_LIBRARIES}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_executable (${UNITTEST_CE} ${TEST_CE_SOURCES})
target_link_libraries(${UNITTEST_CE}
    ${LIBGLOG_LIBRARIES}
    ${GTEST_DIR}/../build/googlemock/gtest/libgtest.a
    ${SYSTEM_EXTERNAL_LIBS})

add_test(NAME DATABASE_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DATABASE})
add_test(NAME DEV_INT_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DEV_INT})
add_test(NAME DEV_RUN_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_DEV_RUN})
add_test(NAME MESSAGE_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_MESSAGE})
add_test(NAME METRICS_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_METRICS})
add_test(NAME CE_TEST WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_CE})
add_test(NAME UNITTEST_EXCHANGE WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${UNITTEST_EXCHANGE})

