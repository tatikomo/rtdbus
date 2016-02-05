set(WORKER_TESTER wtester)
set(WORKER_ECHO   wecho)
set(WORKER_DIGGER wdigger)
set(WORKER_ARCHIVIST warchivist)
set(WORKER_HIST_SAMPLER wsampler)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        rt
        uuid)
  # XSD Synthethis
  include_directories("/usr/local/include")
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
#  set (SYSTEM_EXTERNAL_LIBS
#        pthread)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBSQLITE_INCLUDE_DIR})
include_directories(${LIBZMQ_INCLUDE_DIR})
include_directories(${LIBCPPZMQ_INCLUDE_DIR})
include_directories(${LIBPROTOBUF_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${WORKER_DIR})
include_directories(${MDP_DIR})
include_directories(${MSG_DIR})
include_directories(${HDB_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${TOOLS_DIR})

link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBSQLITE_LIBRARIES_DIR})
link_directories(${LIBZMQ_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
# каталог генерации libmdp.so
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

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
        ${TOOLS_DIR}/helper.cpp
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

set (WORKER_ARCHIVIST_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${WORKER_DIR}/warchivist.cpp
        ${WORKER_DIR}/warchivist.hpp)

set (WORKER_HIST_SAMPLER_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${MDP_DIR}/mdp_zmsg.hpp
        ${MDP_DIR}/mdp_common.h
        ${MDP_DIR}/mdp_worker_api.hpp
        ${MSG_DIR}/msg_message.hpp
        ${WORKER_DIR}/wrk_hist_sampler.cpp
        ${WORKER_DIR}/wrk_hist_sampler.hpp)


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
    ${LIBZMQ_LIBRARIES})

# Сборка ECHO
##########################################################
add_executable (${WORKER_ECHO} ${WORKER_ECHO_SOURCES})
target_link_libraries(${WORKER_ECHO}
    ${MDP_LIBRARY_NAME}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBGLOG_LIBRARIES}
    ${LIBZMQ_LIBRARIES})

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
    ${SYSTEM_EXTERNAL_LIBS}
    ${LIBGLOG_LIBRARIES}
    ${LIBZMQ_LIBRARIES})

#add_definitions(-fpermissive)
# Сборка ARCHIVIST - сервера выдачи архивных данных
##########################################################
add_executable (${WORKER_ARCHIVIST} ${WORKER_ARCHIVIST_SOURCES})
target_link_libraries(${WORKER_ARCHIVIST}
    ${LIBGLOG_LIBRARIES}
#    ${XDB_RTAP_IMPL_LIBRARY_NAME}
#    ${XDB_RTAP_LIBRARY_NAME}
    ${MDP_LIBRARY_NAME}
#    ${TOOLS_LIBRARY_NAME}
#    ${LIBEXTREMEDB_LIBRARIES_ALL}
#    ${LIBPROTOBUF_LIBRARIES}
#    ${SYSTEM_EXTERNAL_LIBS}
#    pthread
#    ${LIBZMQ_LIBRARIES}
    )

# Сборка HIST_SAMPLER - сервера накопления архивных данных
##########################################################
add_executable (${WORKER_HIST_SAMPLER} ${WORKER_HIST_SAMPLER_SOURCES})
target_link_libraries(${WORKER_HIST_SAMPLER}
    ${LIBGLOG_LIBRARIES}
    ${LIBSQLITE_LIBRARIES}
    ${MDP_LIBRARY_NAME}
    ${HDB_LIBRARY_NAME}
    ${XDB_RTAP_LIBRARY_NAME}
    ${LIBPROTOBUF_LIBRARIES}
    )
