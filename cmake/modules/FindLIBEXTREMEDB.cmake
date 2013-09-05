# - Try to find the EXTREMEDB library
# Once done this will define
#
#  LIBEXTREMEDB_FOUND - system has eXtremeDB
#  LIBEXTREMEDB_INCLUDE_DIR - the EXTREMEDB include directories
#  LIBEXTREMEDB_LIBRARIES - The libraries needed to use EXTREMEDB

#  set (EXTREMEDB_VERSION 35)
  set (EXTREMEDB_VERSION 50)

  if ("${EXTREMEDB_VERSION}"  EQUAL "35")
    set (LIBEXTREMEDB_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_3.5/sources/include")
    set (LIBEXTREMEDB_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_3.5/sources/target/bin.so")
    set (LIBEXTREMEDB_MCOCOMP "${RTDBUS_SOURCE_DIR}/../../eXtremeDB_3.5/sources/host/bin/mcocomp")

    if (${COMPILER_PLATFORM} MATCHES "SunOS")
      # SOLARIS EXTREMEDB 3.5
      set (LIBEXTREMEDB_LIBRARIES "mcossun mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcossun -lmcounrt -lmcovtmem -lmcouwrt")
    elseif (${COMPILER_PLATFORM} MATCHES "Linux")
      # LINUX EXTREMEDB 3.5
      set (LIBEXTREMEDB_LIBRARIES "mcolib_shm mcoslnx mcounrt mcovtmem mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_shm_debug -lmcoslnx_debug -lmcounrt_debug -lmcovtmem_debug -lmcouwrt_debug")
    else ()
      message("Unsupported platform ${COMPILER_PLATFORM}")
      return()
    endif()

  elseif ("${EXTREMEDB_VERSION}" EQUAL "41")

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
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcomipc -lmcolib -lmcossun -lmcounrt -lmcovtmem -lmcouwrt -lmcotmursiw")
    elseif (${COMPILER_PLATFORM} MATCHES "Linux")
      # LINUX EXTREMEDB 5.0 EVALUATION
      set (LIBEXTREMEDB_LIBRARIES "mcolib mcovtdsk mcofu98 mcoslnx mcomipc mcotmvcc mcolib mcouwrt")
      set (LIBEXTREMEDB_LIBRARIES_ALL "mcolib_debug -lmcovtdsk_debug -lmcofu98_debug -lmcoslnx_debug -lmcomipc_debug -lmcotmvcc_debug -lmcolib_debug -lmcouwrt_debug")
    endif()

    if (${USE_EXTREMEDB_HTTP_SERVER})
      message("Internal eXtremeDB http server ON")
      set (LIBEXTREMEDB_LIBRARIES "${LIBEXTREMEDB_LIBRARIES} mcouda mcohv mcoews_cgi")
      set (LIBEXTREMEDB_LIBRARIES_ALL "${LIBEXTREMEDB_LIBRARIES_ALL} -lmcohv_debug -lmcouda_debug -lmcoews_cgi_debug")
    else ()
      message("Internal eXtremeDB http server OFF")
    endif ()

  endif ()

  set (LIBEXTREMEDB_FOUND TRUE)
