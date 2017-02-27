if ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # SOLARIS
  set (SYSTEM_EXTERNAL_LIBS
        socket
        nsl
        pthread
        rt)
  # XSD Synthethis
  include_directories("/usr/local/include")
else ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
  # LINUX
  #link_directories(/lib/i386-linux-gnu/)
  set (SYSTEM_EXTERNAL_LIBS
        nsl
        pthread
        rt)
endif ("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")

include_directories(${MDP_DIR})
include_directories(${TOOLS_DIR})
include_directories(${XDB_COMMON_DIR})
include_directories(${XDB_IMPL_DIR})
include_directories(${XDB_BROKER_DIR})
include_directories(${LIBGLOG_INCLUDE_DIR})
include_directories(${LIBEXTREMEDB_INCLUDE_DIR})
include_directories(${XERCESC_INCLUDE_DIR})

link_directories(${LIBEXTREMEDB_LIBRARIES_DIR})
link_directories(${LIBPROTOBUF_LIBRARIES_DIR})
link_directories(${XERCESC_LIBRARIES_DIR})

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

set(XDB_RTAP_DRIVER_DICT_NAME   "driver-dict")
set(XDB_RTAP_DRIVER_DB_NAME     "driver-db")

# Генерация шаблонов и заглушек для функций создания/чтения/записи атрибутов точек
add_custom_command(
        OUTPUT 
        ${XDB_IMPL_DIR}/dat/proto_attr_creating.gen
        ${XDB_IMPL_DIR}/dat/proto_attr_reading.gen
        ${XDB_IMPL_DIR}/dat/proto_attr_writing.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_creating_map.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_reading_map.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_writing_map.gen
        ${XDB_IMPL_DIR}/dat/impl_func_read.gen
        ${XDB_IMPL_DIR}/dat/impl_func_write.gen
        COMMAND
        ${CMAKE_SOURCE_DIR}/gen_xdb_impl_stubs.sh
        DEPENDS
        ${XDB_IMPL_DIR}/dat/attr_list.txt
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap.cpp
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

# Команда вида:
# xsd cxx-parser --generate-validation --generate-noop-impl rtap_db.xsd
# будет дополнительно генерировать заглушки реализации (rtap_db-pimpl.*)
# перезаписать старые "pimpl.[hc]xx" можно флагом --force-overwrite
# создать тестовый код, использующий сгенерированные классы: --generate-test-driver
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

set (XDB_RTAP_DRIVER_DICT_SOURCES
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-driver.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pimpl.cxx)

set (XDB_RTAP_DRIVER_DB_SOURCES
        ${XDB_IMPL_DIR}/dat/rtap_db-driver.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.cxx)

set (XDB_RTAP_IMPL_LIBRARY_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${TOOLS_DIR}/tool_time.hpp
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db-pimpl.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pskel.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pimpl.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db_dict-pimpl.cxx
        ${XDB_IMPL_DIR}/dat/rtap_db.h
        ${XDB_IMPL_DIR}/dat/rtap_db.hpp
        ${XDB_IMPL_DIR}/dat/rtap_db.hxx
        ${XDB_IMPL_DIR}/dat/rtap_db.c
        ${XDB_IMPL_DIR}/xdb_impl_database.cpp
        ${XDB_IMPL_DIR}/xdb_impl_database.hpp
        ${XDB_IMPL_DIR}/xdb_impl_application.cpp
        ${XDB_IMPL_DIR}/xdb_impl_application.hpp
        ${XDB_IMPL_DIR}/xdb_impl_common.c
        ${XDB_IMPL_DIR}/xdb_impl_common.h
        ${XDB_IMPL_DIR}/xdb_impl_environment.cpp
        ${XDB_IMPL_DIR}/xdb_impl_environment.hpp
        ${XDB_IMPL_DIR}/xdb_impl_connection.cpp
        ${XDB_IMPL_DIR}/xdb_impl_connection.hpp
        ${XDB_IMPL_DIR}/xdb_impl_error.cpp
        ${XDB_IMPL_DIR}/xdb_impl_error.hpp
        ${XDB_IMPL_DIR}/dat/proto_attr_creating.gen
        ${XDB_IMPL_DIR}/dat/proto_attr_reading.gen
        ${XDB_IMPL_DIR}/dat/proto_attr_writing.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_creating_map.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_reading_map.gen
        ${XDB_IMPL_DIR}/dat/impl_attr_writing_map.gen
        ${XDB_IMPL_DIR}/dat/impl_func_read.gen
        ${XDB_IMPL_DIR}/dat/impl_func_write.gen
        ${XDB_IMPL_DIR}/xdb_impl_sbs_rtap.cpp
        ${XDB_IMPL_DIR}/xdb_impl_sbs_rtap.hpp
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap_query.cpp
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap_ce.cpp
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap_ce.hpp
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap.cpp
        ${XDB_IMPL_DIR}/xdb_impl_db_rtap.hpp)

set (XDB_BROKER_IMPL_LIBRARY_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.hpp
        ${XDB_COMMON_DIR}/xdb_common.cpp
        ${XDB_COMMON_DIR}/xdb_common.hpp
        ${XDB_IMPL_DIR}/xdb_impl_common.c
        ${XDB_IMPL_DIR}/xdb_impl_common.h
        ${XDB_IMPL_DIR}/xdb_impl_database.cpp
        ${XDB_IMPL_DIR}/xdb_impl_database.hpp
        ${XDB_IMPL_DIR}/xdb_impl_error.cpp
        ${XDB_IMPL_DIR}/xdb_impl_error.hpp
        ${XDB_IMPL_DIR}/dat/broker_db.h
        ${XDB_IMPL_DIR}/dat/broker_db.c
        ${XDB_IMPL_DIR}/dat/broker_db.hpp
        ${XDB_IMPL_DIR}/xdb_impl_db_broker.cpp
        ${XDB_IMPL_DIR}/xdb_impl_db_broker.hpp)

#add_definitions(-fpermissive)

# Тестовый исполняемый драйвер для проверки валидности rtap_dict.xsd
add_executable (${XDB_RTAP_DRIVER_DICT_NAME} ${XDB_RTAP_DRIVER_DICT_SOURCES})
target_link_libraries(${XDB_RTAP_DRIVER_DICT_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${XERCESC_LIBRARIES_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

# Тестовый исполняемый драйвер для проверки валидности rtap_db.xsd
add_executable (${XDB_RTAP_DRIVER_DB_NAME} ${XDB_RTAP_DRIVER_DB_SOURCES})
target_link_libraries(${XDB_RTAP_DRIVER_DB_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${XERCESC_LIBRARIES_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

add_library (${XDB_RTAP_IMPL_LIBRARY_NAME} ${XDB_RTAP_IMPL_LIBRARY_SOURCES})
target_link_libraries(${XDB_RTAP_IMPL_LIBRARY_NAME}
    ${LIBEXTREMEDB_LIBRARIES_ALL}
    ${LIBEXTREMEDB_EXTRA_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

add_library (${XDB_BROKER_IMPL_LIBRARY_NAME} ${XDB_BROKER_IMPL_LIBRARY_SOURCES})
target_link_libraries(${XDB_BROKER_IMPL_LIBRARY_NAME}
    ${LIBEXTREMEDB_EXTRA_LIBRARIES}
    ${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

