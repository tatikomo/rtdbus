# - Try to find the ZMQ library
# Once done this will define
#
#  LIBZMQ_FOUND - system has ZMQ
#  LIBZMQ_INCLUDES - the ZMQ include directories
#  LIBZMQ_LIBRARIES - The libraries needed to use ZMQ

SET (LIBZMQ_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../zeromq-3.2.2/src")
SET (LIBZMQ_LIBRARIES_DIR "${RTDBUS_SOURCE_DIR}/../../zeromq-3.2.2/src/.libs")
SET (LIBZMQ_LIBRARIES "zmq")
SET (LIBZMQ_FOUND TRUE)

