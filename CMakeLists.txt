cmake_minimum_required(VERSION 3.18)


#set_property(GLOBAL PROPERTY USE_FOLDERS ON)


project(jkl VERSION 0.1.0 LANGUAGES CXX)


add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}) # add alias so the project can be uses with add_subdirectory


include(GNUInstallDirs)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
) # NOTE: space can not present in condition of generator expression. The result string will be between :...>, unchanged, including any space.
  #       for file path, cmake recognizes path starting with space as relative path.
  #       if any space is among result string, the whole result string will be quoted, you should use ';' to seprate them.
  #       so you should generally avoid spaces in generator expression.
  # the generator expressions are merely a string, which will be evaluated when build being generated, you can put multiple generator expressions in single string


option($${PROJECT_NAME}_BUNDLE_certify "use bundled certify lib" ON)

if(${PROJECT_NAME}_BUNDLE_certify)
    target_include_directories(${PROJECT_NAME}
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bundled/certify>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/bundled/certify>
    )
endif()


find_package(Boost 1.71.0 REQUIRED)# COMPONENTS asio beast uuid)
find_package(CURL REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc)

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        Boost::headers
        ${Boost_LIBRARIES}
        $<$<PLATFORM_ID:Linux>:-lpthread>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-stdlib=libc++;-lc++abi>
        CURL::libcurl
        ICU::uc
)
#target_include_directories(${PROJECT_NAME} INTERFACE ${Boost_INCLUDE_DIRS})

# RapidJSON
find_package(RapidJSON REQUIRED)
target_include_directories(${PROJECT_NAME} INTERFACE ${RAPIDJSON_INCLUDE_DIRS})

# robin-hood-hashing
find_path(Robin_hood_hashing_INCLUDE_DIRS "robin_hood.h" REQUIRED)
target_include_directories(${PROJECT_NAME} INTERFACE ${Robin_hood_hashing_INCLUDE_DIRS})

# cppcodec
find_path(Cppcodec_INCLUDE_DIRS "cppcodec/base32_crockford.hpp" REQUIRED)
target_include_directories(${PROJECT_NAME} INTERFACE ${Cppcodec_INCLUDE_DIRS})

# gumbo
find_path(Gumbo_INCLUDE_DIR "robin_hood.h" REQUIRED)
find_library(Gumbo_LIBRARIES gumbo REQUIRED)
target_include_directories(${PROJECT_NAME} INTERFACE ${Gumbo_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} INTERFACE ${Gumbo_LIBRARIES})


target_compile_definitions(${PROJECT_NAME}
    INTERFACE
        RAPIDJSON_HAS_STDSTRING
        RAPIDJSON_HAS_CXX11_NOEXCEPT
        RAPIDJSON_HAS_CXX11_TYPETRAITS
        RAPIDJSON_HAS_CXX11_RANGE_FOR
        RAPIDJSON_HAS_CXX11_RVALUE_REFS
)


target_compile_options(${PROJECT_NAME}
    INTERFACE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-std=c++2a;-stdlib=libc++;-Xclang -fconcepts-ts>
        $<$<CXX_COMPILER_ID:GNU>:-std=c++2a;-fcoroutines;-Wno-missing-field-initializers>
        $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
) # NOTE: space can not present in condition of generator expression. The result string will be between :...>, unchanged, including any space.
  #       for file path, cmake recognizes path starting with space as relative path.
  #       if any space is among result string, the whole result string will be quoted, you should use ';' to seprate them.
  #       so you should generally avoid spaces in generator expression.
  # the generator expressions are merely a string, which will be evaluated when build being generated, you can put multiple generator expressions in single string



# if this is the main project, not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # install

    include(CMakePackageConfigHelpers)

    set(_PROJ_CMAKE_CONFIG_DIR ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME})

    configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION
            ${_PROJ_CMAKE_CONFIG_DIR}
    )

    write_basic_package_version_file(
        ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        COMPATIBILITY
            SameMajorVersion
        ARCH_INDEPENDENT
    )

    install(
        TARGETS              ${PROJECT_NAME}
        EXPORT               ${PROJECT_NAME}Targets
        # LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR}
        # ARCHIVE DESTINATION  ${CMAKE_INSTALL_LIBDIR}
        # RUNTIME DESTINATION  ${CMAKE_INSTALL_BINDIR}
        # INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        EXPORT      ${PROJECT_NAME}Targets
        NAMESPACE   ${PROJECT_NAME}::
        DESTINATION ${_PROJ_CMAKE_CONFIG_DIR}
    )

    install(
        DIRECTORY   ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        FILES
            ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
            ${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION
            ${_PROJ_CMAKE_CONFIG_DIR}
    )

    # install bundled

    # if(${PROJECT_NAME}_BUNDLE_certify)
    #     install(
    #         DIRECTORY   ${PROJECT_SOURCE_DIR}/bundled/certify
    #         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/bundled
    #     )
    # endif()


    string(CONCAT _STRICT_COMPILE_OPTIONS
        "$<$<CXX_COMPILER_ID:Clang,AppleClang>:"
            "-Wall;-Wextra;-Wformat=2"
        ">"
        "$<$<CXX_COMPILER_ID:GNU>:"
            "-Wall;-Wextra;-Werror;-Wformat=2;"
            "-Wduplicated-cond;-Wduplicated-branches;-Wlogical-op;"
            "-Wrestrict;-Wnull-dereference;"
            "-Wno-unused-local-typedefs;-Wno-reorder;-Wno-switch"
        ">"
        "$<$<CXX_COMPILER_ID:MSVC>:"
            "/W4;/WX;/permissive-;/utf-8"
        ">"
    )# NOTE: space can not present in condition of generator expression. The result string will be between :...>, unchanged, including any space.
     #       for file path, cmake recognizes path starting with space as relative path.
     #       if any space is among result string, the whole result string will be quoted, you should use ';' to seprate them.
     #       so you should generally avoid spaces in generator expression.
     # the generator expressions are merely a string, which will be evaluated when build being generated, you can put multiple generator expressions in single string

    # optional:
    #   clang : -Wshadow -Wdouble-promotion
    #   gnu: -Wshadow -Wuseless-cast -Wdouble-promotion
    # https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
    # https://clang.llvm.org/docs/DiagnosticsReference.html
    # https://docs.microsoft.com/en-us/cpp/build/reference/compiler-option-warning-level?view=vs-2019


    # test

    option(${PROJECT_NAME}_ENABLE_TEST "enable build tests" ON)

    if(${PROJECT_NAME}_ENABLE_TEST)
        include(CTest)
        add_subdirectory(test)
    endif()

    # benchmark

    option(${PROJECT_NAME}_ENABLE_BENCHMARK "enable build benchmark" ON)

    if(${PROJECT_NAME}_ENABLE_BENCHMARK)
        include(CTest)
        add_subdirectory(benchmark)
    endif()

    # example

    option(${PROJECT_NAME}_ENABLE_EXAMPLE "enable build examples" ON)

    if(${PROJECT_NAME}_ENABLE_EXAMPLE)
        add_subdirectory(example)
    endif()



    # Packaging support

    #set(CPACK_PACKAGE_VENDOR "Vendor name")
    #set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Some summary")
    #set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    #set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    #set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    #set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENCE")
    #set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    #include(CPack)

endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
