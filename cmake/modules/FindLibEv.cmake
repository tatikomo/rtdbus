# - Try to find the EV library
# Once done this will define
#
#  LIBEV_FOUND - system has EV
#  LIBEV_INCLUDES - the EV include directories
#  LIBEV_LIBRARIES - The libraries needed to use EV

if (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)

    # in cache already
    SET(LIBEV_FOUND TRUE)

else (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)

  SET (LIBEV_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../libev-4.11")
  SET (LIBEV_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../libev-4.11/.libs")
  SET (LIBEV_LIBRARIES "ev")
  SET (LIBEV_FOUND TRUE)
  MARK_AS_ADVANCED(LIBEV_INCLUDE_DIR LIBEV_LIBRARIES)

endif (LIBEV_INCLUDE_DIR AND LIBEV_LIBRARIES)
