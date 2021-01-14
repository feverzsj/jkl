find_package(doctest CONFIG REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_path(NANOBENCH_INCLUDE_DIRS "nanobench.h" REQUIRED)

add_executable(benchmark_all benchmark_all.cpp benchmark_message1_proto2.pb.cc)

target_include_directories(benchmark_all PRIVATE ${NANOBENCH_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(benchmark_all PRIVATE ${_STRICT_COMPILE_OPTIONS})
target_link_libraries(benchmark_all PRIVATE jkl doctest::doctest protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)
#set_target_properties(benchmark_all PROPERTIES LINK_FLAGS "-stdlib=libc++ -lc++abi")


add_test(NAME benchmark_all COMMAND benchmark_all)
