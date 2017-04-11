# - Try to find the ZMQ library
# Once done this will define
#
#  LIBZMQ_FOUND - system has ZMQ
#  LIBZMQ_INCLUDES - the ZMQ include directories
#  LIBZMQ_LIBRARIES - The libraries needed to use ZMQ

SET (LIBZMQ_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../libzmq/include")
SET (LIBZMQ_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../libzmq/cmake/lib")
SET (LIBCPPZMQ_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../cppzmq")
#SET (LIBZMQ_LIBRARIES "libzmq") CentOS
SET (LIBZMQ_LIBRARIES "zmq")
SET (LIBZMQ_FOUND TRUE)

