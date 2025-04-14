function(add_custom_executable target_name)
    add_executable(${target_name} ${ARGN})

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/cpplings"
    )
    target_link_libraries(${target_name} PRIVATE pthread)
endfunction()

add_custom_executable(cas cas.cpp)
add_custom_executable(condition_variable condition_variable.cpp)
add_custom_executable(lambda lambda.cpp)
add_custom_executable(lock lock.cpp)
add_custom_executable(mutex mutex.cpp)
add_custom_executable(smart_pointer smart_pointer.cpp)
add_custom_executable(template template.cpp)
add_custom_executable(stl stl.cpp)
