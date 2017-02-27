# - Try to find the RAPIDJSON library : https://github.com/miloyip/rapidjson/
# Once done this will define
#
#  RAPIDJSON_FOUND - system has RAPIDJSON
#  RAPIDJSON_INCLUDES - the RAPIDJSON include directories

#if (RAPIDJSON_INCLUDE_DIR)

    # in cache already
#    SET(RAPIDJSON_FOUND TRUE)

#else (RAPIDJSON_INCLUDE_DIR)

  SET (RAPIDJSON_INCLUDE_DIR "${RTDBUS_SOURCE_DIR}/../../rapidjson/include")
  SET (RAPIDJSON_FOUND TRUE)

#endif (RAPIDJSON_INCLUDE_DIR)
