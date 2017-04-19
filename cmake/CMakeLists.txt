cmake_minimum_required(VERSION 2.6)

project(RTDBUS CXX C)

set (RTDBUS_PROJECT_PREFIX "rtdb")
set (RTDBUS_MAJOR_VERSION "0")
set (RTDBUS_MINOR_VERSION "0")
set (RTDBUS_MICRO_VERSION "5")
set (RTDBUS_VERSION ${RTDBUS_MAJOR_VERSION}.${RTDBUS_MINOR_VERSION}.${RTDBUS_MICRO_VERSION})
set (RTDBUS_VERSION_STRING "${RTDBUS_VERSION}")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/modules")

set (USE_EXTREMEDB_HTTP_SERVER 0)
OPTION (RTDBUS_USE_EXTREMEDB_HTTP_SERVER "enable internal http server"  OFF)
OPTION (RTDBUS_USE_MSEC_IN_EXPIRATION    "enable nanoseconds comparing" OFF)
OPTION (RTDBUS_USE_XDB_CALC              "enable internal xdb statisic" OFF)
 
STRING(TOUPPER ${CMAKE_SYSTEM_NAME} sysname)

if ("${sysname}" MATCHES "SUNOS")
    SET (COMPILER_PLATFORM "SunOS")
    OPTION(HAVE_CONSOLE_OWNER_FILE "enable console owner file (solaris only)" ON)

    if (CMAKE_COMPILER_IS_GNUCC)
      add_definitions(-std=c++11)
      add_definitions(-m32)
    else (CMAKE_COMPILER_IS_GNUCC)
#     add_definitions(-xarch=v9)
      add_definitions(+w2)
      add_definitions(-pg)
      add_definitions("-Qoption ccfe -complextmplexp")
      add_definitions(-library=stlport4)
      # replace internal gcc function for branch execution prediction
      add_definitions(\"-D__builtin_expect\(e,n\)=\(\(e\)\)\")
    endif (CMAKE_COMPILER_IS_GNUCC)

    add_definitions(-DTYPE_OS=SOLARIS)

    if(HAVE_CONSOLE_OWNER_FILE)
        SET(DBUS_CONSOLE_OWNER_FILE "/dev/console" CACHE STRING "Directory to check for console ownerhip")
    endif(HAVE_CONSOLE_OWNER_FILE)

else ("${sysname}" MATCHES "SUNOS")

    SET (COMPILER_PLATFORM "Linux")
    add_definitions(-DTYPE_OS=LINUX)

    if (CMAKE_COMPILER_IS_GNUCC)
      # Проверить тип ОС {Ubuntu | CentOS} и версию GCC
      # GEV: CentOS строит по умолчанию 64-битные версии, а нам нужны 32-битные
      set (CMAKE_C_FLAGS "-m32")
      set (CMAKE_CXX_FLAGS "-m32")

      if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.3")
        # Старая версия не поддерживает расширения С++11
        add_definitions(-std=c++0x)

      else (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.3")
        # Расширенные директивы компилятора
        add_definitions(-std=c++11)
      endif (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8.3")

	  add_definitions(-m32)
	  add_definitions(-Wall)
	  add_definitions(-Wextra)
	  add_definitions(-Wno-unused-parameter)

#  add_definitions(-pedantic)
#  add_definitions(-Weffc++)
#  add_definitions(-O2)
#  add_definitions(-fno-strict-overflow)
#  add_definitions(-fstrict-aliasing)
#  add_definitions(-Wcast-qual)
#  add_definitions(-Wcast-align)
#  add_definitions(-Wwrite-strings)
#  add_definitions(-Wconversion)
#  add_definitions(-Waggregate-return)
#  add_definitions(-Wstrict-null-sentinel)
#  add_definitions(-Wold-style-cast)
#  add_definitions(-Woverloaded-virtual)
#  add_definitions(-Wsign-promo)

    else (CMAKE_COMPILER_IS_GNUCC)
      add_definitions(-xarch=v9)
      add_definitions(+w2)
    endif (CMAKE_COMPILER_IS_GNUCC)

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

find_package(XERCES-C)
if(NOT XERCESC_FOUND)
    message(SEND_ERROR "Failed to find XERCES-C facility")
    return()
endif(NOT XERCESC_FOUND)

find_package(LIBSQLITE)
if(NOT LIBSQLITE_FOUND)
    message(SEND_ERROR "Failed to find SQLite facility")
    return()
endif(NOT LIBSQLITE_FOUND)

find_package(LIBMODBUS)
if(NOT LIBMODBUS_FOUND)
    message(SEND_ERROR "Failed to find MODBUS facility")
    return()
endif(NOT LIBMODBUS_FOUND)

find_package(RAPIDJSON)
if(NOT RAPIDJSON_FOUND)
    message(SEND_ERROR "Failed to find RAPIDJSON headers")
    return()
endif(NOT RAPIDJSON_FOUND)

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
	set(RTDBUS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${RTDBUS_PROJECT_PREFIX}")
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
set(XDB_IMPL_DIR             ${CMAKE_SOURCE_DIR}/../xdb/impl)
set(XDB_RTAP_DIR             ${CMAKE_SOURCE_DIR}/../xdb/rtap)
set(XDB_COMMON_DIR           ${CMAKE_SOURCE_DIR}/../xdb/common)
set(XDB_BROKER_DIR           ${CMAKE_SOURCE_DIR}/../xdb/broker)
set(TOOLS_DIR                ${CMAKE_SOURCE_DIR}/../tools)
set(WORKER_DIR               ${CMAKE_SOURCE_DIR}/../worker)
set(CLIENT_DIR               ${CMAKE_SOURCE_DIR}/../client)
set(MSG_DIR                  ${CMAKE_SOURCE_DIR}/../msg)
set(HDB_DIR                  ${CMAKE_SOURCE_DIR}/../hdb)
set(EXCHANGE_DIR             ${CMAKE_SOURCE_DIR}/../exchange)
set(EXCHANGE_SA_DIR          ${EXCHANGE_DIR}/sa)
set(EXCHANGE_CONFIG_DIR      ${EXCHANGE_DIR}/config)
set(EXCHANGE_SMAD_DIR        ${EXCHANGE_DIR}/smad)
set(EXCHANGE_EGSA_DIR        ${EXCHANGE_DIR}/egsa)
set(EXCHANGE_LIB_DIR         ${EXCHANGE_DIR}/lib)

set(MDP_LIBRARY_NAME             ${RTDBUS_PROJECT_PREFIX}_mdp_${RTDBUS_VERSION})
# Высокоуровневая библиотека работы с БД RTAP, отвязанная от деталей реализации MCO
set(XDB_RTAP_LIBRARY_NAME        ${RTDBUS_PROJECT_PREFIX}_xrtap_${RTDBUS_VERSION})
# Низкоуровневые детали работы с БД RTAP
set(XDB_RTAP_IMPL_LIBRARY_NAME   ${RTDBUS_PROJECT_PREFIX}_xrtapi_${RTDBUS_VERSION})
# Высокоуровневая библиотека работы с БД Брокера, отвязанная от деталей реализации MCO
set(XDB_BROKER_LIBRARY_NAME      ${RTDBUS_PROJECT_PREFIX}_xbrkr_${RTDBUS_VERSION})
# Низкоуровневые детали работы с БД Брокера
set(XDB_BROKER_IMPL_LIBRARY_NAME ${RTDBUS_PROJECT_PREFIX}_xbrkri_${RTDBUS_VERSION})
# Реализация
set(XDB_RTAP_DIGGER_NAME         ${RTDBUS_PROJECT_PREFIX}_xdig_${RTDBUS_VERSION})
# Классы сообщений
set(MSG_LIBRARY_NAME             ${RTDBUS_PROJECT_PREFIX}_zmsg_${RTDBUS_VERSION})
# Вспомогательные неспецифичные функции
set(TOOLS_LIBRARY_NAME           ${RTDBUS_PROJECT_PREFIX}_hlpr_${RTDBUS_VERSION})
# Библиотека для ведения архива значений и работы с ними
set(HDB_LIBRARY_NAME             ${RTDBUS_PROJECT_PREFIX}_hdb_${RTDBUS_VERSION})
# Функции для работы с конфигурационными файлами систем сбора и EGSA
set(EXCHANGE_LIBRARY_NAME        ${RTDBUS_PROJECT_PREFIX}_exchg_${RTDBUS_VERSION})
# Реализация обертки над MODBUS, содержащая специфичное API для RTDBUS 
set(MODBUS_LIBRARY_NAME          ${RTDBUS_PROJECT_PREFIX}_mbus_${RTDBUS_VERSION})

OPTION(RTDBUS_BUILD_TESTS "enable unit test code" ON)
OPTION(RTDBUS_ENABLE_VERBOSE_MODE "Enable additional debug trace" ON)

# ограничение размера полей данных из схемы MCO (Примечание: в БАЙТАХ!)
set (MAX_SERVICES_ENTRY      400)
set (MAX_WORKERS_ENTRY       800) # WORKERS_SPOOL_SIZE * MAX_SERVICES_ENTRY
set (ENDPOINT_MAXLEN         100)
set (SERVICE_NAME_MAXLEN      32)
set (TAG_NAME_MAXLEN          32)
set (LABEL_MAXLEN            100)
set (IDENTITY_MAXLEN          11)
set (DBNAME_MAXLEN            10)
# TODO Объединить все подобные интервалы (HEARTBEAT) в единую структуру. 
# Сейчас у Брокера и Обработчиков могут быть разные значения.
set (HEARTBEAT_PERIOD_MSEC  2500)
# Разумный интервал 3..5
set (HEARTBEAT_LIVENESS 3)

# таймауты
set (RECV_TIMEOUT_MSEC   3000000)
set (SEND_TIMEOUT_MSEC   1000000)
set (MESSAGE_EXPIRATION_PERIOD_MSEC  500)

# Зарезервированный размер памяти под БДРВ [Мб]
set (DIGGER_DB_SIZE_MB  10)

# максимальное количество экземпляров различных БД в одной системе одновременно
set (MAX_DATABASE_INSTANCES_PER_SYSTEM 10)

# Для использования точек подключения в серверах следует подстроку
# от последнего символа '/' до символа ':' 
# Значение по-умолчанию точки подключения к Брокеру
set (ENDPOINT_BROKER                "tcp://localhost:5555")
# Значения по-умолчанию точек подключения к Службе БДРВ
set (ENDPOINT_RTDB_FRONTEND         "tcp://localhost:5556")
# Управляющий сокет DiggerProxy
set (ENDPOINT_RTDB_PROXY_CTRL       "tcp://localhost:5557")
# Теневой сокет DiggerProxy <=> DiggerWorker
# NB: транспорт inproc работает между нитями только в пределах единого контекста zmq
set (ENDPOINT_RTDB_DATA_BACKEND     "inproc://rtdb_digger_data")
set (ENDPOINT_RTDB_COMMAND_BACKEND  "inproc://rtdb_digger_command")
set (ENDPOINT_HMI_FRONTEND          "tcp://localhost:5558")
set (ENDPOINT_EXCHG_FRONTEND        "tcp://localhost:5559")
set (ENDPOINT_SBS_PUBLISHER         "tcp://localhost:5560")
set (ENDPOINT_HIST_FRONTEND         "tcp://localhost:5561")

set (HISTDB_SNAP_FILENAME           "hdb.sqlite3")
# Локальное или удаленное расположение БДРВ для службы Архивирования.
# Значение LOCAL - БДРВ расположена на тем же сервере, можно получать данные локально
# Значение REMOTE - БДРВ расположена на удаленном сервере, получать данные можно только запросом
set (HIST_DATABASE_LOCALIZATION     "LOCAL")

# Предопределенные названия Служб
set (BROKER_NAME        "BROKER")
set (RTDB_NAME          "SINF")
set (HMI_NAME           "HMI")
set (EXCHANGE_NAME      "EGSA")
set (HISTORIAN_NAME     "HIST")

# Названия таблиц Исторической БД (HDB)
set (HDB_DATA_TABLENAME "HIST")
set (HDB_DICT_TABLENAME "HIST_DICT")
# Название файла со снимком HDB
set (HISTORY_DB_FILENAME "hdb.sqlite3")
# Максимальное кол-во одновременно читаемых элементов предыстории для одного параметра
set (MAX_PORTION_SIZE_LOADED_HISTORY 100)
# Размер буфера для формирования SQL-команды
set (MAX_BUFFER_SIZE_FOR_SQL_COMMAND 400)

# Максимальное количество одновременно допустимых подключений
set (DEFAULT_MAX_DB_CONNECTIONS_ALLOWED  4)

if(RTDBUS_BUILD_TESTS)
    add_definitions(-DRTDBUS_BUILD_TESTS)
    add_definitions(-DDEBUG)
    enable_testing()
endif(RTDBUS_BUILD_TESTS)

add_definitions(-DMCO_STRICT_ALIGNMENT)
add_definitions(-g)

########### create config.h ###############
add_definitions(-DHAVE_CONFIG_H)

set(RTDBUS_LIBRARIES rtdbus-1)
set(RTDBUS_INTERNAL_LIBRARIES socket nsl stdc++)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rtdbus-env.bat.cmake ${CMAKE_BINARY_DIR}/bin/rtdbus-env.bat )
install_files(/bin FILES ${CMAKE_BINARY_DIR}/bin/rtdbus-env.bat)

add_subdirectory( xdb/impl )
add_subdirectory( xdb/broker )
add_subdirectory( xdb/rtap )
add_subdirectory( hdb )
add_subdirectory( exchange/lib )
add_subdirectory( exchange/egsa )
add_subdirectory( exchange/sa )
add_subdirectory( tools )
add_subdirectory( mdp )
add_subdirectory( worker )
add_subdirectory( client )
add_subdirectory( msg )
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
message(STATUS "cxx compiler version:     ${CMAKE_CXX_COMPILER_VERSION}       ")
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
message(STATUS "sqlite:                   ${LIBSQLITE_LIBRARIES_DIR}\t${LIBSQLITE_LIBRARIES}")
message(STATUS "json:                     ${RAPIDJSON_INCLUDE_DIR}")
message(STATUS "Building unit tests:      ${RTDBUS_BUILD_TESTS}               ")
message(STATUS "Building xdb http serv:   ${RTDBUS_USE_EXTREMEDB_HTTP_SERVER} ")
message(STATUS "Building verbose mode:    ${RTDBUS_ENABLE_VERBOSE_MODE}       ")
message(STATUS "Building w/o assertions:  ${RTDBUS_DISABLE_ASSERTS}           ")
message(STATUS "Building w/o checks:      ${RTDBUS_DISABLE_CHECKS}            ")
message(STATUS "Building XML docs:        ${RTDBUS_ENABLE_XML_DOCS}           ")
message(STATUS "Using XML parser:         ${XML_LIB}                          ")
message(STATUS "Daemon executable name:   ${RTDBUS_DAEMON_NAME}               ")
#message(STATUS "LD_LIBRARY_PATH:          $ENV{LD_LIBRARY_PATH}               ")

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
    add_definitions(-DVERBOSE=9)
endif(RTDBUS_ENABLE_VERBOSE_MODE)

if(NOT RTDBUS_DISABLE_ASSERTS)
    message("NOTE: building with assertions increases library size and decreases performance.")
endif(NOT RTDBUS_DISABLE_ASSERTS)

if (RTDBUS_DISABLE_CHECKS)
    message("NOTE: building without checks for arguments passed to public API makes it harder to debug apps using RTDBUS, but will slightly decrease RTDBUS library size and _very_ slightly improve performance.")
endif(RTDBUS_DISABLE_CHECKS)
MESSAGE(" ")

# Generate TAGS file
add_custom_target(
  tags ALL
  DEPENDS "${RTDBUS_SOURCE_DIR}/tags"
  WORKING_DIRECTORY ${RTDBUS_SOURCE_DIR}
)

add_custom_command(
  OUTPUT "${RTDBUS_SOURCE_DIR}/tags"
  COMMAND "/usr/bin/ctags" "-R"
  WORKING_DIRECTORY ${RTDBUS_SOURCE_DIR}
  COMMENT "Generating TAGS file for VIM"
  VERBATIM
)


