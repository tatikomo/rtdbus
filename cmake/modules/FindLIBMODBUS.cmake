# - Try to find the LIBMODBUS library : https://github.com/stephane/libmodbus
# Once done this will define
#
#  LIBMODBUS_FOUND - system has MODBUS
#  LIBMODBUS_INCLUDES - the MODBUS include directories
#  LIBMODBUS_LIBRARIES - The libraries needed to use MODBUS

#if (LIBMODBUS_INCLUDE_DIR AND LIBMODBUS_LIBRARIES)

    # in cache already
#    SET(LIBMODBUS_FOUND TRUE)

#else (LIBMODBUS_INCLUDE_DIR AND LIBMODBUS_LIBRARIES)

  SET (LIBMODBUS_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../libmodbus/src")
  SET (LIBMODBUS_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../libmodbus/src/.libs")
  SET (LIBMODBUS_LIBRARIES "modbus")
  SET (LIBMODBUS_FOUND TRUE)

#endif (LIBMODBUS_INCLUDE_DIR AND LIBMODBUS_LIBRARIES)
