if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        pthread
        rt)
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        pthread
        rt)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${XDB_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
link_directories(/lib/i386-linux-gnu/)
link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})

set (XDB_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${XDB_DIR}/xdb_database.cpp
        ${XDB_DIR}/xdb_database.hpp
        ${XDB_DIR}/xdb_database_impl.cpp
        ${XDB_DIR}/xdb_database_impl.hpp
        ${XDB_DIR}/xdb_database_broker.cpp
        ${XDB_DIR}/xdb_database_broker.hpp
        ${XDB_DIR}/xdb_database_broker_impl.cpp
        ${XDB_DIR}/xdb_database_broker_impl.hpp
        ${XDB_DIR}/xdb_database_service.cpp
        ${XDB_DIR}/xdb_database_service.hpp
        )

# Сборка библиотеки
##########################################################
#add_definitions("-pthread")
#set (${CMAKE_CXX_FLAGS} "-pthread -lrt" )
#set (${CMAKE_LD_FLAGS} "-pthread -lrt" )
add_library (${XDB_LIBRARY_NAME} ${XDB_SOURCES})
target_link_libraries(${XDB_LIBRARY_NAME}
    ${EXTREMEDB_LIBRARIES_ALL}
    ${SYSTEM_EXTERNAL_LIBS})

