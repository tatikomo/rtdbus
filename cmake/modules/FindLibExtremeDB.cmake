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

  set (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/include")
  set (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/target/bin.so")
  set (LIBEXTREMEDB_MCOCOMP "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/host/bin/mcocomp")

  set (EXTREMEDB_VERSION 35)

  if(${COMPILER_PLATFORM} MATCHES "SunOS")
    if (${EXTREMEDB_VERSION}<=40)
      # for eXtremeDB 3.5 under Solaris
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "lmcossun -lmcounrt -lmcovtmem -lmcouwrt")
    else (${EXTREMEDB_VERSION}<=40)
      # eXtremeDB 4.1 under Solaris
      add_definitions(-DMCO_STRICT_ALIGNMENT)
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcomipc -lmcolib -lmcossun -lmcounrt -lmcovtmem -lmcouwrt -lmcotmursiw")
    endif (${EXTREMEDB_VERSION}<=40)
  else (${COMPILER_PLATFORM} MATCHES "SunOS")
      # for eXtremeDB 3.5 under Linux
      set (LIBEXTREMEDB_LIBRARIES "mcoslnx mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_shm -lmcoslnx -lmcounrt -lmcovtmem -lmcouwrt")
  endif (${COMPILER_PLATFORM} MATCHES "SunOS")



  if(${COMPILER_PLATFORM} MATCHES "SunOS")
    set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
    set (LIBEXTREMEDB_LIBRARIES_ALL "mcossun -lmcounrt -lmcovtmem -lmcouwrt")
  else (${COMPILER_PLATFORM} MATCHES "SunOS")
    set (LIBEXTREMEDB_LIBRARIES "mcoslnx mcounrt mcovtmem mcouwrt")
    set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_shm -lmcoslnx -lmcounrt -lmcovtmem -lmcouwrt")
  endif (${COMPILER_PLATFORM} MATCHES "SunOS")

  set (LIBEXTREMEDB_FOUND TRUE)
  mark_as_advanced(LIBEXTREMEDB_INCLUDE_DIR 
            LIBEXTREMEDB_LIBRARIES_DIR 
            LIBEXTREMEDB_LIBRARIES 
            LIBEXTREMEDB_MCOCOMP)

endif (LIBEXTREMEDB_INCLUDE_DIR AND LIBEXTREMEDB_LIBRARIES)
