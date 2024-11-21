include(CheckCXXCompilerFlag)

file(GLOB MEMTRACER_SOURCES "*.cpp")

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
add_library(memtracer SHARED ${MEMTRACER_SOURCES})
#ignore missing attributes
#ref jemalloc: https://github.com/jemalloc/jemalloc/blob/master/configure.ac
CHECK_CXX_COMPILER_FLAG("-Wno-missing-attributes" COMPILER_SUPPORTS_NO_MISSING_ATTRIBUTES)
if(COMPILER_SUPPORTS_NO_MISSING_ATTRIBUTES)
    target_compile_options(memtracer PRIVATE -Wno-missing-attributes)
endif()

# hidden memtracer internal interfaces
set_target_properties(memtracer PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)
target_link_libraries(memtracer pthread)