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
INCLUDE(readline)
MINIOB_FIND_READLINE()

IF (HAVE_READLINE) 
    TARGET_LINK_LIBRARIES(oblsm_cli ${READLINE_LIBRARY})
    TARGET_INCLUDE_DIRECTORIES(oblsm_cli PRIVATE ${READLINE_INCLUDE_DIR})
    ADD_DEFINITIONS(-DUSE_READLINE)
    MESSAGE ("oblsm_cli use readline")
ELSE ()
    MESSAGE ("readline is not found")
ENDIF()
TARGET_LINK_LIBRARIES(oblsm_cli oblsm)


ADD_EXECUTABLE(oblsm_bench benchmarks/ob_lsm_bench.cpp)
TARGET_LINK_LIBRARIES(oblsm_bench oblsm)