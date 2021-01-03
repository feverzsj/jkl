find_package(doctest CONFIG REQUIRED)

add_executable(test_all test_all.cpp)

target_include_directories(test_all PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(test_all PRIVATE ${_STRICT_COMPILE_OPTIONS})
target_link_libraries(test_all PRIVATE jkl doctest::doctest)
#set_target_properties(test_all PROPERTIES LINK_FLAGS "-stdlib=libc++ -lc++abi")


add_test(NAME test_all COMMAND test_all)
