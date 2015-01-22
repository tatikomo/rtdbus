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

set (TOOLS_SOURCES
        ${CMAKE_SOURCE_DIR}/config.h
        ${TOOLS_DIR}/helper.cpp
        ${TOOLS_DIR}/helper.hpp
        ${TOOLS_DIR}/timer.cpp
        ${TOOLS_DIR}/timer.hpp
        )

if (CMAKE_COMPILER_IS_GNUCC)
#add_definitions (-Wno-missing-field-initializers)
#add_definitions(-fpermissive)
else (CMAKE_COMPILER_IS_GNUCC)
  add_definitions (-library=stlport4)
endif (CMAKE_COMPILER_IS_GNUCC)
add_definitions (-library=stlport4)

add_library (${TOOLS_LIBRARY_NAME} ${TOOLS_SOURCES})
target_link_libraries(${TOOLS_LIBRARY_NAME}
    ${SYSTEM_EXTERNAL_LIBS})

