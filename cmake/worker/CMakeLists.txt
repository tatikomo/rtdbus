set(WORKER_TESTER wtester)
set(WORKER_ECHO   wecho)
set(WORKER_DIGGER wdigger)
set(WORKER_HISTORIAN whistorian)
set(WORKER_SYSACQ_CLI wsysacq_cli)
set(WORKER_EGSA wegsa)
set(WORKER_TIMEKEEPER wtiker)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        rt
        uuid)
  # XSD Synthethis
  include_directories("/usr/local/include")
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  #set (SYSTEM_EXTERNAL_LIBS
  #      rt
  #      pthread)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBSQLITE_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBCPPZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${RAPIDJSON_INCLUDE_DIR})
include_directories(${XERCESC_INCLUDE_DIR})
include_directories(${WORKER_DIR})
include_directories(${EXCHANGE_SA_DIR})
include_directories(${EXCHANGE_SMAD_DIR})
include_directories(${EXCHANGE_CONFIG_DIR})
include_directories(${EXCHANGE_EGSA_DIR})
include_directories(${LIBMODBUS_INCLUDE_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${HDB_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${TOOLS_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBSQLITE_LIBRARIES_DIR})
link_directories(${XERCESC_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBMODBUS_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

add_definitions(-DVERBOSE=6)

set (WORKER_TESTER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wtester.hpp
        ${WORKER_DIR}/wtester.cpp)

set (WORKER_ECHO_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${WORKER_DIR}/wecho.hpp
        ${WORKER_DIR}/wecho.cpp)

set (WORKER_DIGGER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db.h
        ${XDB_IMPL_DIR}/dat/rtap_db.c
        ${WORKER_DIR}/wdigger.cpp
        ${WORKER_DIR}/wdigger.hpp)

set (WORKER_HISTORIAN_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${WORKER_DIR}/whistorian.cpp
        ${WORKER_DIR}/whistorian.hpp)

set (WORKER_SYSACQ_CLI_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${EXCHANGE_SA_DIR}/exchange_sysacq_modbus.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config.hpp
        ${WORKER_DIR}/wsysacq_cli.cpp
        ${WORKER_DIR}/wsysacq_cli.hpp)

set (WORKER_EGSA_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${TOOLS_DIR}/tool_timer.hpp
        ${TOOLS_DIR}/tool_events.hpp
        ${WORKER_DIR}/wegsa.cpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config_egsa.hpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_int.hpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_ext.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_site.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_site.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request_base.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request_cyclic.hpp
        )

set (WORKER_TIMEKEEPER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${WORKER_DIR}/wtimekeeper.cpp
        ${WORKER_DIR}/wtimekeeper.hpp)

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

add_custom_command(
        OUTPUT
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        COMMAND
        /usr/bin/xsd cxx-parser --type-map rtap_db.map --generate-validation rtap_db.xsd
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

set_source_files_properties(${XDB_IMPL_DIR}/dat/rtap_db.hpp
                            ${XDB_IMPL_DIR}/dat/rtap_db.h
                            ${XDB_IMPL_DIR}/dat/rtap_db.c
                            PROPERTIES GENERATED 1)

set_source_files_properties(${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
                            ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
                            PROPERTIES GENERATED 1)

# Сборка тестового торгового обработчика
##########################################################
add_executable (${WORKER_TESTER} ${WORKER_TESTER_SOURCES})
target_link_libraries(${WORKER_TESTER}
    ${MDP_LIBRARY_NAME}
    ${MSG_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBGLOG_LIBRARIES}
    ${LIBZMQ_LIBRARIES}
    -lpthread
    )

# Сборка ECHO
##########################################################
add_executable (${WORKER_ECHO} ${WORKER_ECHO_SOURCES})
target_link_libraries(${WORKER_ECHO}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBGLOG_LIBRARIES}
    ${LIBZMQ_LIBRARIES}
    -lpthread
    )

#add_definitions(-fpermissive)
# Сборка DIGGER - сервера БДРВ
##########################################################
add_executable (${WORKER_DIGGER} ${WORKER_DIGGER_SOURCES})
target_link_libraries(${WORKER_DIGGER}
    ${XDB_RTAP_IMPL_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBPROTOBUF_LIBRARIES}
    ${XERCESC_LIBRARIES_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBGLOG_LIBRARIES}
    ${LIBZMQ_LIBRARIES}
    )

# Сборка HISTORIAN - сервера накопления и выдачи архивных данных
##########################################################
add_executable (${WORKER_HISTORIAN} ${WORKER_HISTORIAN_SOURCES})
target_link_libraries(${WORKER_HISTORIAN}
    ${XERCESC_LIBRARIES_NAME}
    ${LIBGLOG_LIBRARIES}
    ${LIBSQLITE_LIBRARIES}
    ${MDP_LIBRARY_NAME}
    ${HDB_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    )

# Сборка SYSACQ_CLIENT - службы системы сбора
##########################################################
add_executable (${WORKER_SYSACQ_CLI} ${WORKER_SYSACQ_CLI_SOURCES})
target_link_libraries(${WORKER_SYSACQ_CLI}
    ${LIBGLOG_LIBRARIES}
    "pthread"
    ${MDP_LIBRARY_NAME}
    ${MODBUS_LIBRARY_NAME}
    ${LIBMODBUS_LIBRARIES}
    ${LIBPROTOBUF_LIBRARIES}
    )

# Сборка EGSA - модуль диспетчеризации Системами Сбора
add_executable (${WORKER_EGSA} ${WORKER_EGSA_SOURCES})
target_link_libraries(${WORKER_EGSA}
    "pthread"
    ${EXCHANGE_LIBRARY_NAME}
    ${LIBMODBUS_LIBRARIES}
    ${LIBGLOG_LIBRARIES}
    ${LIBPROTOBUF_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
    ${LIBSQLITE_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS}
    )

# Сборка TIMEKEEPER - сервера управления и генерации разовых и повторяющихся событий
##########################################################
add_executable (${WORKER_TIMEKEEPER} ${WORKER_TIMEKEEPER_SOURCES})
target_link_libraries(${WORKER_TIMEKEEPER}
    ${LIBGLOG_LIBRARIES}
    ${LIBSQLITE_LIBRARIES}
    ${MDP_LIBRARY_NAME}
    ${HDB_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    )
