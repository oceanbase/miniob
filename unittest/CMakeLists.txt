# 可以获取父cmake的变量
MESSAGE("${CMAKE_COMMON_FLAGS}")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/observer)

find_package(GTest CONFIG REQUIRED)

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
