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

  SET (LIBGLOG_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../glog-0.3.2/src")
  SET (LIBGLOG_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../glog-0.3.2/.libs")
  SET (LIBGLOG_LIBRARIES "glog")
  SET (LIBGLOG_FOUND TRUE)

#endif (LIBGLOG_INCLUDE_DIR AND LIBGLOG_LIBRARIES)
