FILE(GLOB_RECURSE ALL_SRC *.cpp)

FOREACH (F ${ALL_SRC})
    get_filename_component(prjName ${F} NAME_WE)
    MESSAGE("Build ${prjName} according to ${F}")
    ADD_EXECUTABLE(${prjName} ${F})
    TARGET_LINK_LIBRARIES(${prjName} GTest::gtest_main oblsm)
    add_test(NAME ${prjName} COMMAND ${prjName})
ENDFOREACH (F)
