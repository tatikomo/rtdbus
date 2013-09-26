# - Try to find the PROTOBUF library
# Once done this will define
#
#  LIBPROTOBUF_FOUND - system has PROTOBUF
#  LIBPROTOBUF_INCLUDES - the PROTOBUF include directories
#  LIBPROTOBUF_LIBRARIES - The libraries needed to use PROTOBUF

SET (LIBPROTOBUF_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../protobuf-2.5.0/src")
SET (LIBPROTOBUF_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../protobuf-2.5.0/src/.libs")
SET (PROTOC "${LIBPROTOBUF_LIBRARIES_DIR}/protoc")
SET (LIBPROTOBUF_LIBRARIES "protobuf")
SET (LIBPROTOBUF_FOUND TRUE)
