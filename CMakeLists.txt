cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

project(minidb)

MESSAGE(STATUS "This is Project source dir " ${PROJECT_SOURCE_DIR})
MESSAGE(STATUS "This is PROJECT_BINARY_DIR dir " ${PROJECT_BINARY_DIR})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/deps/3rd/usr/local;/usr/local;${CMAKE_PREFIX_PATH}")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

# 设置默认构建类型为 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type:Default is debug" FORCE)
endif()
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

# 输出当前构建类型
message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")

OPTION(ENABLE_ASAN "Enable build with address sanitizer" ON)
OPTION(ENABLE_TSAN "Build with thread sanitizer" OFF)
OPTION(ENABLE_UBSAN "Build with undefined behavior sanitizer" OFF)
OPTION(WITH_UNIT_TESTS "Compile miniob with unit tests" ON)
OPTION(WITH_BENCHMARK "Compile benchmark" OFF)
# TODO: support MemTracer with sanitizers, currently MemTracer doesn't work with sanitizers.
OPTION(WITH_MEMTRACER "Compile memtracer" OFF)
OPTION(ENABLE_COVERAGE "Enable unittest coverage" OFF)
OPTION(ENABLE_NOPIE "Enable no pie" OFF)
OPTION(CONCURRENCY "Support concurrency operations" OFF)
OPTION(STATIC_STDLIB "Link std library static or dynamic, such as libgcc, libstdc++, libasan" OFF)
OPTION(USE_SIMD "Use SIMD" OFF)
OPTION(USE_MUSL_LIBC "Use musl libc" OFF)
OPTION(WITH_CPPLINGS "Compile cpplings" ON)
OPTION(USE_REPLXX "Use replxx instead of linenoise for line reading" OFF)

MESSAGE(STATUS "HOME dir: $ENV{HOME}")
#SET(ENV{变量名} 值)
IF(WIN32)
    MESSAGE(STATUS "This is windows.")
    ADD_DEFINITIONS(-DWIN32)
ELSEIF(WIN64)
    MESSAGE(STATUS "This is windows.")
    ADD_DEFINITIONS(-DWIN64)
ELSEIF(APPLE)
    MESSAGE(STATUS "This is apple")
    # normally __MACH__ has already been defined
    ADD_DEFINITIONS(-D__MACH__ )
ELSEIF(UNIX)
    MESSAGE(STATUS "This is UNIX")
    ADD_DEFINITIONS(-DUNIX -DLINUX)
ELSE()
    MESSAGE(STATUS "This is UNKNOW OS")
ENDIF(WIN32)

# This is for clangd plugin for vscode
# mute sign-compare error in lex/yacc
SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -Wall -Werror -Wno-error=sign-compare")
IF ("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "debug")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -DDEBUG -g -O0")
ELSEIF ("${CMAKE_BUILD_TYPE_LOWER}" STREQUAL "release")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -g -O2")
ENDIF()

IF (ENABLE_NOPIE)
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -no-pie")
    ADD_LINK_OPTIONS(-no-pie)
ENDIF (ENABLE_NOPIE)

# Requires support for avx2
IF(USE_SIMD)
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -mavx2")
    ADD_DEFINITIONS(-DUSE_SIMD)
ENDIF(USE_SIMD)

IF (CONCURRENCY)
    MESSAGE(STATUS "CONCURRENCY is ON")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -DCONCURRENCY")
    ADD_DEFINITIONS(-DCONCURRENCY)
ENDIF (CONCURRENCY)

MESSAGE(STATUS "CMAKE_CXX_COMPILER_ID is " ${CMAKE_CXX_COMPILER_ID})
IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${STATIC_STDLIB})
    ADD_LINK_OPTIONS(-static-libgcc -static-libstdc++)
ENDIF()

IF(USE_MUSL_LIBC)
    ADD_DEFINITIONS(-D__MUSL__)
    MESSAGE(STATUS "musl libc use pthread in default")
    SET(CMAKE_THREAD_LIBS_INIT "-lpthread")

    MESSAGE(AUTHOR_WARNING "Sanitizer and musl libc not support each other for now")
    SET(ENABLE_ASAN OFF)
ENDIF(USE_MUSL_LIBC)

IF (ENABLE_ASAN)
    MESSAGE(STATUS "Instrumenting with Address Sanitizer")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fno-omit-frame-pointer -fsanitize=address -fsanitize-address-use-after-scope")
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${STATIC_STDLIB})
        ADD_LINK_OPTIONS(-static-libasan)
    ENDIF()
    add_definitions(-DENABLE_ASAN)
ENDIF()

IF (ENABLE_TSAN)
    # supported flags
    # https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags
    MESSAGE(STATUS "Instrumenting with Thread Sanitizer")
    SET(TSAN_FLAGS "-fno-omit-frame-pointer -fsanitize=thread")
    SET(TSAN_CCFLAGS "${TSAN_CCFLAGS} -mllvm -tsan-instrument-memory-accesses=0")
    SET(TSAN_CCFLAGS "${TSAN_CCFLAGS} -mllvm -tsan-instrument-atomics=0")
    SET(TSAN_CCFLAGS "${TSAN_CCFLAGS} -mllvm -tsan-instrument-func-entry-exit=1")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} ${TSAN_FLAGS} ${TSAN_CCFLAGS}")
    # -Qunused-arguments 有些编译器不支持，所以先删掉
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND ${STATIC_STDLIB})
        ADD_LINK_OPTIONS(-static-libtsan)
    ENDIF()
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${TSAN_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TSAN_FLAGS}")
ENDIF (ENABLE_TSAN)

IF (ENABLE_UBSAN)
    # Only success on Mac Clang
    MESSAGE(STATUS "Instrumenting with Undefined Behavior Sanitizer")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fno-omit-frame-pointer")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=undefined")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=implicit-conversion")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=implicit-integer-truncation")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=integer")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=nullability")
    SET(UBSAN_FLAGS "${UBSAN_FLAGS} -fsanitize=vptr")
    SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} ${UBSAN_FLAGS}")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${UBSAN_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${UBSAN_FLAGS}")
ENDIF (ENABLE_UBSAN)

IF (CMAKE_INSTALL_PREFIX)
    MESSAGE(STATUS "CMAKE_INSTALL_PREFIX has been set as " ${CMAKE_INSTALL_PREFIX} )
ELSEIF(DEFINED ENV{CMAKE_INSTALL_PREFIX})
    SET(CMAKE_INSTALL_PREFIX $ENV{CMAKE_INSTALL_PREFIX})
ELSE()
    SET(CMAKE_INSTALL_PREFIX /tmp/${PROJECT_NAME})
ENDIF()
MESSAGE(STATUS "Install target dir is " ${CMAKE_INSTALL_PREFIX})

IF (DEFINED ENV{LD_LIBRARY_PATH})
    SET(LD_LIBRARY_PATH_STR $ENV{LD_LIBRARY_PATH})
    string(REPLACE ":" ";" LD_LIBRARY_PATH_LIST ${LD_LIBRARY_PATH_STR})
    MESSAGE(" Add LD_LIBRARY_PATH to -L flags " ${LD_LIBRARY_PATH_LIST})
    LINK_DIRECTORIES(${LD_LIBRARY_PATH_LIST})
ENDIF ()

IF (EXISTS /usr/local/lib)
    LINK_DIRECTORIES (/usr/local/lib)
ENDIF ()
IF (EXISTS /usr/local/lib64)
    LINK_DIRECTORIES (/usr/local/lib64)
ENDIF ()

INCLUDE_DIRECTORIES(. ${PROJECT_SOURCE_DIR}/deps ${PROJECT_SOURCE_DIR}/deps/3rd/usr/local/include ${PROJECT_SOURCE_DIR}/src /usr/local/include)

IF(WITH_UNIT_TESTS)
    IF (ENABLE_COVERAGE)
        SET(CMAKE_COMMON_FLAGS "${CMAKE_COMMON_FLAGS} -fprofile-arcs -ftest-coverage -fprofile-update=atomic")
    ENDIF (ENABLE_COVERAGE)
    enable_testing()
ENDIF(WITH_UNIT_TESTS)

SET(CMAKE_CXX_FLAGS ${CMAKE_COMMON_FLAGS})
SET(CMAKE_C_FLAGS ${CMAKE_COMMON_FLAGS})

MESSAGE(STATUS "CMAKE_CXX_FLAGS is " ${CMAKE_CXX_FLAGS})

# Line Reading configs
IF(USE_REPLXX)
    ADD_DEFINITIONS(-DUSE_REPLXX=1)
    MESSAGE(STATUS "Using replxx for line reading")
    FIND_PACKAGE(Threads REQUIRED)
    FIND_PACKAGE(replxx REQUIRED)
ELSE()
    ADD_DEFINITIONS(-DUSE_REPLXX=0)
    MESSAGE(STATUS "Using linenoise for line reading")
ENDIF()

# ADD_SUBDIRECTORY(src bin)  bin 为目标目录， 可以省略
# ADD_SUBDIRECTORY(deps)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tools)

IF (WITH_BENCHMARK)
    ADD_SUBDIRECTORY(benchmark)
ENDIF(WITH_BENCHMARK)

IF(WITH_UNIT_TESTS)
    ADD_SUBDIRECTORY(unittest)
ENDIF(WITH_UNIT_TESTS)
