file(GLOB_RECURSE OBLSM_SOURCES "*.cpp")
file(GLOB CLIUTIL_SOURCES "client/cliutil/*.cpp")
list(FILTER OBLSM_SOURCES EXCLUDE REGEX "client/.*")
list(FILTER OBLSM_SOURCES EXCLUDE REGEX "benchmarks/.*")

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
add_library(oblsm STATIC ${OBLSM_SOURCES})
message("OBLSM_SOURCES: ${OBLSM_SOURCES}")

FIND_PACKAGE(jsoncpp CONFIG REQUIRED)

TARGET_LINK_LIBRARIES(oblsm common pthread JsonCpp::JsonCpp)

ADD_EXECUTABLE(oblsm_cli ${CLIUTIL_SOURCES} client/ob_lsm_client.cpp)

TARGET_LINK_LIBRARIES(oblsm_cli oblsm)

ADD_EXECUTABLE(oblsm_bench benchmarks/ob_lsm_bench.cpp)
TARGET_LINK_LIBRARIES(oblsm_bench oblsm)
