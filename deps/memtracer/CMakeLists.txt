file(GLOB MEMTRACER_SOURCES "*.cpp")


set(CMAKE_CXX_VISIBILITY_PRESET hide)
add_library(memtracer SHARED ${MEMTRACER_SOURCES})
#ignore missing attributes
#ref jemalloc: https://github.com/jemalloc/jemalloc/blob/master/configure.ac
CHECK_CXX_COMPILER_FLAG("-Wno-missing-attributes" COMPILER_SUPPORTS_NO_MISSING_ATTRIBUTES)
if(COMPILER_SUPPORTS_NO_MISSING_ATTRIBUTES)
    target_compile_options(memtracer PRIVATE -Wno-missing-attributes)
endif()
set_target_properties(memtracer PROPERTIES
    CXX_VISIBILITY_PRESET hidden # 默认隐藏所有符号
    VISIBILITY_INLINES_HIDDEN YES # 内联函数也隐藏
)
target_link_libraries(memtracer pthread)