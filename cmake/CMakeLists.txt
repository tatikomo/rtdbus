cmake_minimum_required(VERSION 2.6)

project(RTDBUS CXX C)

set (RTDBUS_MAJOR_VERSION "0")
set (RTDBUS_MINOR_VERSION "0")
set (RTDBUS_MICRO_VERSION "2")
set (RTDBUS_VERSION ${RTDBUS_MAJOR_VERSION}.${RTDBUS_MINOR_VERSION}.${RTDBUS_MICRO_VERSION})
set (RTDBUS_VERSION_STRING "${RTDBUS_VERSION}")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

STRING(TOUPPER ${CMAKE_SYSTEM_NAME} sysname)
if("${sysname}" MATCHES "SUNOS")
    SET (COMPILER_PLATFORM "SunOS")
    OPTION(HAVE_CONSOLE_OWNER_FILE "enable console owner file (solaris only)" ON)
    add_definitions(-DTYPE_OS=SOLARIS)
    if(HAVE_CONSOLE_OWNER_FILE)
        SET(DBUS_CONSOLE_OWNER_FILE "/dev/console" CACHE STRING "Directory to check for console ownerhip")
    endif(HAVE_CONSOLE_OWNER_FILE)
else ("${sysname}" MATCHES "SUNOS")
    SET (COMPILER_PLATFORM "Linux")
    add_definitions(-DTYPE_OS=LINUX)
endif("${sysname}" MATCHES "SUNOS")

set (BUILD_SHARED_LIBS ON)

include (FindPkgConfig)
find_package(LIBZMQ)
if(NOT LIBZMQ_FOUND)
    message(SEND_ERROR "Failed to find ZeroMQ facility")
    return()
endif(NOT LIBZMQ_FOUND)

find_package(LIBPROTOBUF REQUIRED)
if(NOT LIBPROTOBUF_FOUND)
    message(SEND_ERROR "Failed to find Google protobuf facility")
    return()
endif(NOT LIBPROTOBUF_FOUND)

find_package(LIBEXTREMEDB REQUIRED)
if(NOT LIBEXTREMEDB_FOUND)
    message(SEND_ERROR "Failed to find eXtremeDB facility")
    return()
endif(NOT LIBEXTREMEDB_FOUND)

find_package(LIBGLOG)
if(NOT LIBGLOG_FOUND)
    message(SEND_ERROR "Failed to find Google LOG facility")
    return()
endif(NOT LIBGLOG_FOUND)

include (ConfigureChecks.cmake)
include (MacroLibrary)

set (RTDBUS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/..)
set (LIBRARY_OUTPUT_PATH  ${CMAKE_BINARY_DIR}/lib)
set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# for including config.h and for includes like <dir/foo.h>
include_directories(${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR} ${CMAKE_INCLUDE_PATH})
#include_directories(${CMAKE_LIBRARY_PATH})

# linker search directories
link_directories(${RTDBUS_LIB_DIR} ${LIBRARY_OUTPUT_PATH})
link_directories(${CMAKE_SOURCE_DIR}/../cmake/lib)

#set(RTDBUS_INCLUDES )

########### basic vars ###############
enable_testing()

if (RTDBUSDIR)
	set(RTDBUS_INSTALL_DIR "${DBUSDIR}")
endif (RTDBUSDIR)

if ($ENV{RTDBUSDIR})
	set(RTDBUS_INSTALL_DIR "$ENV{RTDBUSDIR}")
endif ($ENV{RTDBUSDIR})

if (RTDBUS_INSTALL_DIR)
	set(CMAKE_INSTALL_PREFIX "${RTDBUS_INSTALL_DIR}" CACHE TYPE PATH FORCE)
else (RTDBUS_INSTALL_DIR)
	set(RTDBUS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/rtdbus")
endif (RTDBUS_INSTALL_DIR)

set(prefix                   ${RTDBUS_INSTALL_DIR})
set(exec_prefix              ${prefix})
set(EXPANDED_LIBDIR          ${RTDBUS_INSTALL_DIR}/lib)
set(EXPANDED_INCLUDEDIR      ${RTDBUS_INSTALL_DIR}/include)
set(EXPANDED_BINDIR          ${RTDBUS_INSTALL_DIR}/bin)
set(EXPANDED_SYSCONFDIR      ${RTDBUS_INSTALL_DIR}/etc)
set(EXPANDED_DATADIR         ${RTDBUS_INSTALL_DIR}/data)
set(RTDBUS_BINDIR            ${EXPANDED_BINDIR})
set(RTDBUS_DAEMONDIR         ${EXPANDED_BINDIR})

# добавил для тестов - где задавать структуру каталога исходников?
set(MDP_DIR                  ${CMAKE_SOURCE_DIR}/../mdp)
set(XDB_DIR                  ${CMAKE_SOURCE_DIR}/../database)
set(TOOLS_DIR                ${CMAKE_SOURCE_DIR}/../tools)
set(WORKER_DIR               ${CMAKE_SOURCE_DIR}/../worker)
set(CLIENT_DIR               ${CMAKE_SOURCE_DIR}/../client)
set(MSG_DIR                  ${CMAKE_SOURCE_DIR}/../msg)
set(MDP_LIBRARY_NAME         mdp)
set(XDB_LIBRARY_NAME         xdb)
set(MSG_LIBRARY_NAME         zmsg)
set(TOOLS_LIBRARY_NAME       rtdb_helper)

OPTION(RTDBUS_BUILD_TESTS "enable unit test code" ON)
# ограничение размера полей данных из схемы MCO
set (MAX_SERVICES_ENTRY      400)
set (MAX_WORKERS_ENTRY       800) # WORKERS_SPOOL_SIZE * MAX_SERVICES_ENTRY
set (SERVICE_NAME_MAXLEN      32)
set (WORKER_IDENTITY_MAXLEN   11)
set (DBNAME_MAXLEN            10)
# TODO Объединить все подобные интервалы (HEARTBEAT) в единую структуру. 
# Сейчас у Брокера и Обработчиков могут быть разные значения.
set (HEARTBEAT_PERIOD_MSEC  2500)

set (WORKERS_SPOOL_SIZE        2)
set (MESSAGE_EXPIRATION_PERIOD_MSEC  500)
#set (USE_EXTREMEDB_HTTP_SERVER 1)

OPTION (RTDBUS_USE_EXTREMEDB_HTTP_SERVER "enable internal http server"  OFF)
OPTION (RTDBUS_USE_MSEC_IN_EXPIRATION    "enable nanoseconds comparing" OFF)
 
if(RTDBUS_BUILD_TESTS)
    add_definitions(-DRTDBUS_BUILD_TESTS)
    add_definitions(-DDEBUG)
    enable_testing()
endif(RTDBUS_BUILD_TESTS)

add_definitions(-DMCO_STRICT_ALIGNMENT)
add_definitions(-g)
if (CMAKE_COMPILER_IS_GNUCC)
#  add_definitions(-mcpu=v9)
#  add_definitions(-G)
  add_definitions(-m32)
  add_definitions(-Wall)
#  add_definitions(-Wextra)
else (CMAKE_COMPILER_IS_GNUCC)
  add_definitions(-xarch=v9)
  add_definitions(+w2)
endif (CMAKE_COMPILER_IS_GNUCC)

########### create config.h ###############
add_definitions(-DHAVE_CONFIG_H)

set(RTDBUS_LIBRARIES rtdbus-1)
set(RTDBUS_INTERNAL_LIBRARIES socket nsl stdc++)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rtdbus-env.bat.cmake ${CMAKE_BINARY_DIR}/bin/rtdbus-env.bat )
install_files(/bin FILES ${CMAKE_BINARY_DIR}/bin/rtdbus-env.bat)

add_subdirectory( tools )
add_subdirectory( mdp )
add_subdirectory( worker )
add_subdirectory( client )
add_subdirectory( msg )
add_subdirectory( database )
if (RTDBUS_BUILD_TESTS)
  add_subdirectory( test )
endif (RTDBUS_BUILD_TESTS)

GET_FILENAME_COMPONENT(C_COMPILER ${CMAKE_C_COMPILER} NAME)
GET_FILENAME_COMPONENT(CXX_COMPILER ${CMAKE_CXX_COMPILER} NAME)

message("                  RTD-BUS ${RTDBUS_VERSION}                           ")
message("                  ==============                                      ")
message("                                                                      ")
message(STATUS "System/Version:           ${CMAKE_SYSTEM_NAME} / ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Cmake generates:          ${CMAKE_GENERATOR}                  ")
message(STATUS "install prefix:           ${prefix}                           ")
message(STATUS "install exec_prefix:      ${exec_prefix}                      ")
message(STATUS "install libdir:           ${EXPANDED_LIBDIR}                  ")
message(STATUS "install bindir:           ${EXPANDED_BINDIR}                  ")
message(STATUS "install sysconfdir:       ${EXPANDED_SYSCONFDIR}              ")
message(STATUS "install datadir:          ${EXPANDED_DATADIR}                 ")
message(STATUS "source code location:     ${RTDBUS_SOURCE_DIR}                ")
message(STATUS "build dir:                ${CMAKE_BINARY_DIR}                 ")
message(STATUS "c compiler:               ${C_COMPILER}                       ")
message(STATUS "cflags:                   ${CMAKE_C_FLAGS}                    ")
message(STATUS "cflags debug:             ${CMAKE_C_FLAGS_DEBUG}              ")
message(STATUS "cflags release:           ${CMAKE_C_FLAGS_RELEASE}            ")
message(STATUS "cxx compiler:             ${CXX_COMPILER}                     ")
message(STATUS "cxxflags:                 ${CMAKE_CXX_FLAGS}                  ")
message(STATUS "cxxflags debug:           ${CMAKE_CXX_FLAGS_DEBUG}            ")
message(STATUS "cxxflags release:         ${CMAKE_CXX_FLAGS_RELEASE}          ")
message(STATUS "64-bit int:               ${RTDBUS_INT64_TYPE}                ")
message(STATUS "32-bit int:               ${RTDBUS_INT32_TYPE}                ")
message(STATUS "16-bit int:               ${RTDBUS_INT16_TYPE}                ")
message(STATUS "ZeroMQ:                   ${LIBZMQ_LIBRARIES_DIR}\t${LIBZMQ_LIBRARIES}")
message(STATUS "Protobuf:                 ${LIBPROTOBUF_LIBRARIES_DIR}\t${LIBPROTOBUF_LIBRARIES}")
message(STATUS "eXtremeDB:                ${LIBEXTREMEDB_LIBRARIES_DIR}\t${LIBEXTREMEDB_LIBRARIES}")
message(STATUS "glog:                     ${LIBGLOG_LIBRARIES_DIR}\t${LIBGLOG_LIBRARIES}")
message(STATUS "Building unit tests:      ${RTDBUS_BUILD_TESTS}               ")
message(STATUS "Building xdb http serv:   ${RTDBUS_USE_EXTREMEDB_HTTP_SERVER} ")
message(STATUS "Building verbose mode:    ${RTDBUS_ENABLE_VERBOSE_MODE}       ")
message(STATUS "Building w/o assertions:  ${RTDBUS_DISABLE_ASSERTS}           ")
message(STATUS "Building w/o checks:      ${RTDBUS_DISABLE_CHECKS}            ")
message(STATUS "Building XML docs:        ${RTDBUS_ENABLE_XML_DOCS}           ")
message(STATUS "Using XML parser:         ${XML_LIB}                          ")
message(STATUS "Daemon executable name:   ${RTDBUS_DAEMON_NAME}               ")

MESSAGE(" ")
if (RTDBUS_BUILD_TESTS)
    message("NOTE: building with unit tests increases the size of the installed library and renders it insecure.")
endif(RTDBUS_BUILD_TESTS)

if (RTDBUS_BUILD_TESTS AND RTDBUS_DISABLE_ASSERTS)
    message("NOTE: building with unit tests but without assertions means tests may not properly report failures (this configuration is only useful when doing something like profiling the tests)")
endif(RTDBUS_BUILD_TESTS AND RTDBUS_DISABLE_ASSERTS)

if (RTDBUS_GCOV_ENABLED)
    message("NOTE: building with coverage profiling is definitely for developers only.")
endif(RTDBUS_GCOV_ENABLED)

if (RTDBUS_ENABLE_VERBOSE_MODE)
    message("NOTE: building with verbose mode increases library size, may slightly increase security risk, and decreases performance.")
endif(RTDBUS_ENABLE_VERBOSE_MODE)

if(NOT RTDBUS_DISABLE_ASSERTS)
    message("NOTE: building with assertions increases library size and decreases performance.")
endif(NOT RTDBUS_DISABLE_ASSERTS)

if (RTDBUS_DISABLE_CHECKS)
    message("NOTE: building without checks for arguments passed to public API makes it harder to debug apps using D-BUS, but will slightly decrease D-BUS library size and _very_ slightly improve performance.")
endif(RTDBUS_DISABLE_CHECKS)
MESSAGE(" ")
