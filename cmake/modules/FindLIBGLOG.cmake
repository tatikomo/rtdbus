# - Try to find the GLOG library
# Once done this will define
#
#  LIBGLOG_FOUND - system has GLOG
#  LIBGLOG_INCLUDES - the GLOG include directories
#  LIBGLOG_LIBRARIES - The libraries needed to use GLOG

#if (LIBGLOG_INCLUDE_DIR AND LIBGLOG_LIBRARIES)

    # in cache already
#    SET(LIBGLOG_FOUND TRUE)

#else (LIBGLOG_INCLUDE_DIR AND LIBGLOG_LIBRARIES)

  SET (LIBGLOG_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../glog/src")
  SET (LIBGLOG_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../glog/.libs")
  SET (LIBGLOG_LIBRARIES "libglog.a")
  SET (LIBGLOG_FOUND TRUE)

#endif (LIBGLOG_INCLUDE_DIR AND LIBGLOG_LIBRARIES)
