# - Try to find the EXTREMEDB library
# Once done this will define
#
#  LIBEXTREMEDB_FOUND - system has GLIB
#  LIBEXTREMEDB_INCLUDE_DIR - the EXTREMEDB include directories
#  LIBEXTREMEDB_LIBRARIES - The libraries needed to use EXTREMEDB

if (LIBEXTREMEDB_INCLUDE_DIR AND LIBEXTREMEDB_LIBRARIES)

    # in cache already
    SET(LIBEXTREMEDB_FOUND TRUE)

else (LIBEXTREMEDB_INCLUDE_DIR AND LIBEXTREMEDB_LIBRARIES)

  SET (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/include")
  SET (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/target/bin.so")

if(${COMPILER_PLARFORM} MATCHES "SunOS")
  SET (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
  SET (LIBEXTREMEDB_LIBRARIES_ALL "mcossun -lmcounrt -lmcovtmem -lmcouwrt")
else (${COMPILER_PLARFORM} MATCHES "SunOS")
  SET (LIBEXTREMEDB_LIBRARIES "mcoslnx mcounrt mcovtmem mcouwrt")
  SET (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_shm -lmcoslnx -lmcounrt -lmcovtmem -lmcouwrt")
endif (${COMPILER_PLARFORM} MATCHES "SunOS")

  SET (LIBEXTREMEDB_FOUND TRUE)
  MARK_AS_ADVANCED(LIBEXTREMEDB_INCLUDE_DIR LIBEXTREMEDB_LIBRARIES_DIR LIBEXTREMEDB_LIBRARIES)

endif (LIBEXTREMEDB_INCLUDE_DIR AND LIBEXTREMEDB_LIBRARIES)
