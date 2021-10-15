PROJECT(unitest)
MESSAGE("Begin to build " ${PROJECT_NAME})
MESSAGE(STATUS "This is PROJECT_BINARY_DIR dir " ${PROJECT_BINARY_DIR})
MESSAGE(STATUS "This is PROJECT_SOURCE_DIR dir " ${PROJECT_SOURCE_DIR})


# 可以获取父cmake的变量
MESSAGE("${CMAKE_COMMON_FLAGS}")


#INCLUDE_DIRECTORIES([AFTER|BEFORE] [SYSTEM] dir1 dir2 ...)
INCLUDE_DIRECTORIES(. ${PROJECT_SOURCE_DIR}/../deps ${PROJECT_SOURCE_DIR}/../src/observer /usr/local/include SYSTEM)
# 父cmake 设置的include_directories 和link_directories并不传导到子cmake里面
#INCLUDE_DIRECTORIES(BEFORE ${CMAKE_INSTALL_PREFIX}/include)
LINK_DIRECTORIES(/usr/local/lib /usr/local/lib64 ${PROJECT_BINARY_DIR}/../lib)


IF (DEFINED ENV{LD_LIBRARY_PATH})
    SET(LD_LIBRARY_PATH_STR $ENV{LD_LIBRARY_PATH})
    #separate_arguments(LD_LIBRARY_PATH_STR)  #只能处理空行
    string(REPLACE ":" ";" LD_LIBRARY_PATH_LIST ${LD_LIBRARY_PATH_STR})
    MESSAGE(" Add LD_LIBRARY_PATH to -L flags " ${LD_LIBRARY_PATH_LIST})
    LINK_DIRECTORIES(${LD_LIBRARY_PATH_LIST})
ELSE ()
    LINK_DIRECTORIES(/usr/local/lib)
ENDIF ()


find_package(GTest CONFIG REQUIRED)


enable_testing()
include(GoogleTest)
#get_filename_component(<VAR> FileName
#        PATH|ABSOLUTE|NAME|EXT|NAME_WE|REALPATH
#        [CACHE])
FILE(GLOB_RECURSE ALL_SRC *.cpp)
# AUX_SOURCE_DIRECTORY 类似功能
FOREACH (F ${ALL_SRC})
    get_filename_component(prjName ${F} NAME_WE)
    MESSAGE("Build ${prjName} according to ${F}")
    ADD_EXECUTABLE(${prjName} ${F})
    # 不是所有的单测都需要链接observer_static
    TARGET_LINK_LIBRARIES(${prjName} common pthread dl gtest gtest_main observer_static)
    gtest_discover_tests(${prjName})
ENDFOREACH (F)

