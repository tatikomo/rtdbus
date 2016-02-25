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
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_RTAP_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${LIBSQLITE_INCLUDE_DIR})

link_directories(${LIBSQLITE_LIBRARIES_DIR})
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (HDB_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${HDB_DIR}/hdb_impl_processor.cpp
        ${HDB_DIR}/hdb_impl_processor.hpp
        )

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-Wno-missing-field-initializers)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
  add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)
#add_definitions (-DVERBOSE=1)

add_library (${HDB_LIBRARY_NAME} ${HDB_SOURCES})
target_link_libraries(${HDB_LIBRARY_NAME}
    ${LIBSQLITE_LIBRARIES}
    ${SYSTEM_EXTERNAL_LIBS})

