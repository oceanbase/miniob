FILE(GLOB_RECURSE ALL_SRC *.cpp)

FIND_PACKAGE(jsoncpp CONFIG REQUIRED)

FOREACH (F ${ALL_SRC})
    get_filename_component(prjName ${F} NAME_WE)
    MESSAGE("Build ${prjName} according to ${F}")
    ADD_EXECUTABLE(${prjName} ${F})
    TARGET_LINK_LIBRARIES(${prjName} pthread GTest::gtest_main JsonCpp::JsonCpp oblsm)
    add_test(NAME ${prjName} COMMAND ${prjName})
ENDFOREACH (F)
