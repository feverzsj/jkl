find_package(doctest CONFIG REQUIRED)

add_executable(test1 test1.cpp)

target_compile_options(test1 PRIVATE ${_STRICT_COMPILE_OPTIONS})
target_link_libraries(test1 PRIVATE jkl doctest::doctest)
#set_target_properties(test1 PROPERTIES LINK_FLAGS "-stdlib=libc++ -lc++abi")

add_test(NAME test1 COMMAND test1)
