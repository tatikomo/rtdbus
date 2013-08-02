# - Try to find the EXTREMEDB library
# Once done this will define
#
#  LIBEXTREMEDB_FOUND - system has eXtremeDB
#  LIBEXTREMEDB_INCLUDE_DIR - the EXTREMEDB include directories
#  LIBEXTREMEDB_LIBRARIES - The libraries needed to use EXTREMEDB

#  set (EXTREMEDB_VERSION 35)
  set (EXTREMEDB_VERSION 50)

  if ("${EXTREMEDB_VERSION}"  EQUAL "35")
    set (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/include")
    set (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/target/bin.so")
    set (LIBEXTREMEDB_MCOCOMP "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_3.5.987_sunos/sources/host/bin/mcocomp")

    if (${COMPILER_PLATFORM} MATCHES "SunOS")
      # SOLARIS EXTREMEDB 3.5
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcossun -lmcounrt -lmcovtmem -lmcouwrt")
    elseif (${COMPILER_PLATFORM} MATCHES "Linux")
      # LINUX EXTREMEDB 3.5
      set (LIBEXTREMEDB_LIBRARIES "mcolib mcovtdsk mcofu98 mcoslnx mcomipc mcotmvcc mcolib mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_debug -lmcovtdsk_debug -lmcofu98_debug -lmcoslnx_debug -lmcomipc_debug -lmcotmvcc_debug -lmcolib_debug -lmcouwrt_debug")
    else ()
      message("Unsupported platform ${COMPILER_PLATFORM}")
      return()
    endif()

  elseif ("${EXTREMEDB_VERSION}" EQUAL "41")
message("31")

    set (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_41/sources/include")
    set (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_41/sources/target/bin.so")
    set (LIBEXTREMEDB_MCOCOMP "${RTDBUS_SOURCE_DIR}/../../master_eXtremeDB_41/sources/host/bin/mcocomp")

    if (${COMPILER_PLATFORM} MATCHES "SunOS")
      # SOLARIS EXTREMEDB 4.1
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcossun -lmcounrt -lmcovtmem -lmcouwrt")
    else ()
      message("Unsupported combination platform ${COMPILER_PLATFORM} and eXtremeDB version ${EXTREMEDB_VERSION}")
      return()
    endif ()

  elseif ("${EXTREMEDB_VERSION}" EQUAL "50")

    message("WARNING: YOU USE EVALUATION VERSION OF EXTREMEDB!")
    set (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_5_fusion_eval/include")
    set (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_5_fusion_eval/target/bin.so")
    set (LIBEXTREMEDB_MCOCOMP "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_5_fusion_eval/host/bin/mcocomp")

    if (${COMPILER_PLATFORM} MATCHES "SunOS")
      # SOLARIS EXTREMEDB 5.0 EVALUATION
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt mcohv")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcomipc -lmcolib -lmcossun -lmcounrt -lmcovtmem -lmcouwrt -lmcotmursiw -lmcohv_debug")
    elseif (${COMPILER_PLATFORM} MATCHES "Linux")
      # LINUX EXTREMEDB 5.0 EVALUATION
      set (LIBEXTREMEDB_LIBRARIES "mcolib mcovtdsk mcofu98 mcoslnx mcomipc mcotmvcc mcolib mcouwrt mcouda mcohv mcoews_cgi")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_debug -lmcovtdsk_debug -lmcofu98_debug -lmcoslnx_debug -lmcomipc_debug -lmcotmvcc_debug -lmcolib_debug -lmcouwrt_debug -lmcouda_debug  -lmcohv_debug -lmcoews_cgi_debug")
    endif()

  endif ()

  set (LIBEXTREMEDB_FOUND TRUE)
