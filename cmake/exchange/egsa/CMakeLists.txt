set(MODULE_NAME egsa)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        rt)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  link_directories(/lib/i386-linux-gnu/)
  set (SYSTEM_EXTERNAL_LIBS
        rt)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${TOOLS_DIR})
include_directories(${EXCHANGE_CONFIG_DIR})
include_directories(${EXCHANGE_SMAD_DIR})
include_directories(${XDB_COMMON_DIR})
#include_directories(${XDB_RTAP_DIR})
#include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBSQLITE_INCLUDE_DIR})
include_directories(${LIBMODBUS_INCLUDE_DIR})
include_directories(${RAPIDJSON_INCLUDE_DIR})

link_directories(${LIBMODBUS_LIBRARIES_DIR})
link_directories(${LIBGLOG_LIBRARIES_DIR})
link_directories(${LIBSQLITE_LIBRARIES_DIR})
link_directories(${EXCHANGE_LIB_DIR})
#link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (MODULE_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${TOOLS_DIR}/tool_timer.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config.hpp
        ${EXCHANGE_CONFIG_DIR}/exchange_config_egsa.hpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_int.hpp
        ${EXCHANGE_SMAD_DIR}/exchange_smad_ext.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_init.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_impl.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_sa.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_main.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.cpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_cycle.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request_base.hpp
        ${EXCHANGE_EGSA_DIR}/exchange_egsa_request_cyclic.hpp
        )

#if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-Wno-missing-field-initializers)
#add_definitions(-fpermissive)
#else (CMAKE_COMPILER_IS_GNUCC)
#  add_definitions (-library=stlport4)
#endif (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-library=stlport4)
add_definitions (-DVERBOSE=9)

add_executable (${MODULE_NAME} ${MODULE_SOURCES})
target_link_libraries(${MODULE_NAME}
    ${EXCHANGE_LIBRARY_NAME}
    ${LIBMODBUS_LIBRARIES}
    ${LIBGLOG_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${LIBSQLITE_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

