cmake_minimum_required(VERSION 3.20)
project(muesli VERSION 0.0.0 LANGUAGES CXX)

# Enforce C++20 globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

add_library(muesli INTERFACE)

target_include_directories(muesli INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(muesli INTERFACE cxx_std_20)

if(BUILD_TESTING)
    include(CTest)
    enable_testing()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)

    macro(add_muesli_test_exe target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE muesli)
        list(APPEND MUESLI_TEST_TARGETS ${target})
    endmacro()

    add_muesli_test_exe(test_fundamental_codecs tests/test_fundamental_codecs.cpp)
    add_test(NAME test_fundamental_codecs COMMAND test_fundamental_codecs)

    add_muesli_test_exe(test_stdint_codecs tests/test_stdint_codecs.cpp)
    add_test(NAME test_stdint_codecs COMMAND test_stdint_codecs)

    add_muesli_test_exe(test_stddef_codecs tests/test_stddef_codecs.cpp)
    add_test(NAME test_stddef_codecs COMMAND test_stddef_codecs)

    add_muesli_test_exe(test_identity_codec tests/test_identity_codec.cpp)
    add_test(NAME test_identity_codec COMMAND test_identity_codec)

    add_muesli_test_exe(test_apply_codec tests/test_apply_codec.cpp)
    add_test(NAME test_apply_codec COMMAND test_apply_codec)

    add_muesli_test_exe(test_constant_codec tests/test_constant_codec.cpp)
    add_test(NAME test_constant_codec COMMAND test_constant_codec)

    add_muesli_test_exe(test_monostate_codec tests/test_monostate_codec.cpp)
    add_test(NAME test_monostate_codec COMMAND test_monostate_codec)

    add_muesli_test_exe(test_transform_codec tests/test_transform_codec.cpp)
    add_test(NAME test_transform_codec COMMAND test_transform_codec)

    add_muesli_test_exe(test_varint_codecs tests/test_varint_codecs.cpp)
    add_test(NAME test_varint_codecs COMMAND test_varint_codecs)

    add_muesli_test_exe(test_array_codec tests/test_array_codec.cpp)
    add_test(NAME test_array_codec COMMAND test_array_codec)

    add_muesli_test_exe(test_vector_codec tests/test_vector_codec.cpp)
    add_test(NAME test_vector_codec COMMAND test_vector_codec)

    add_muesli_test_exe(test_range_codec tests/test_range_codec.cpp)
    add_test(NAME test_range_codec COMMAND test_range_codec)

    add_muesli_test_exe(test_tuple_codec tests/test_tuple_codec.cpp)
    add_test(NAME test_tuple_codec COMMAND test_tuple_codec)

    add_muesli_test_exe(test_variant_codec tests/test_variant_codec.cpp)
    add_test(NAME test_variant_codec COMMAND test_variant_codec)

    add_muesli_test_exe(test_optional_codec tests/test_optional_codec.cpp)
    add_test(NAME test_optional_codec COMMAND test_optional_codec)

    add_muesli_test_exe(test_nullable_codec tests/test_nullable_codec.cpp)
    add_test(NAME test_nullable_codec COMMAND test_nullable_codec)

    add_muesli_test_exe(test_binary_format tests/test_binary_format.cpp)
    add_test(NAME test_binary_format COMMAND test_binary_format)

    add_muesli_test_exe(test_constrained_codec tests/test_constrained_codec.cpp)
    add_test(NAME test_constrained_codec COMMAND test_constrained_codec)

    add_muesli_test_exe(test_string_codecs tests/test_string_codecs.cpp)
    add_test(NAME test_string_codecs COMMAND test_string_codecs)

    add_muesli_test_exe(test_range_holder tests/test_range_holder.cpp)
    add_test(NAME test_range_holder COMMAND test_range_holder)

    add_muesli_test_exe(test_member_codec tests/test_member_codec.cpp)
    add_test(NAME test_member_codec COMMAND test_member_codec)

    add_muesli_test_exe(test_projector_codec tests/test_projector_codec.cpp)
    add_test(NAME test_projector_codec COMMAND test_projector_codec)

    add_muesli_test_exe(test_delimited_codec tests/test_delimited_codec.cpp)
    add_test(NAME test_delimited_codec COMMAND test_delimited_codec)

    add_muesli_test_exe(test_fluent_constrain tests/test_fluent_constrain.cpp)
    add_test(NAME test_fluent_constrain COMMAND test_fluent_constrain)

    add_muesli_test_exe(test_fluent_optional tests/test_fluent_optional.cpp)
    add_test(NAME test_fluent_optional COMMAND test_fluent_optional)

    add_muesli_test_exe(test_fluent_nullable tests/test_fluent_nullable.cpp)
    add_test(NAME test_fluent_nullable COMMAND test_fluent_nullable)

    add_muesli_test_exe(test_or_else_codec tests/test_or_else_codec.cpp)
    add_test(NAME test_or_else_codec COMMAND test_or_else_codec)

    add_muesli_test_exe(test_json_format tests/test_json_format.cpp)
    add_test(NAME test_json_format COMMAND test_json_format)

    add_muesli_test_exe(test_schema tests/test_schema.cpp)
    add_test(NAME test_schema COMMAND test_schema)

    include(FetchContent)

    # nlohmann/json
    option(MUESLI_TEST_NLOHMANN_JSON "Build nlohmann/json backend tests" ON)
    if(MUESLI_TEST_NLOHMANN_JSON)
        FetchContent_Declare(nlohmann_json
            GIT_REPOSITORY https://github.com/nlohmann/json.git
            GIT_TAG        v3.11.3
            GIT_SHALLOW    TRUE
        )
        set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(nlohmann_json)

        add_muesli_test_exe(test_json_nlohmann tests/test_json_nlohmann.cpp)
        target_link_libraries(test_json_nlohmann PRIVATE nlohmann_json::nlohmann_json)
        add_test(NAME test_json_nlohmann COMMAND test_json_nlohmann)

        add_muesli_test_exe(test_named_member tests/test_named_member.cpp)
        target_link_libraries(test_named_member PRIVATE nlohmann_json::nlohmann_json)
        add_test(NAME test_named_member COMMAND test_named_member)

        add_muesli_test_exe(test_json_or_else tests/test_json_or_else.cpp)
        target_link_libraries(test_json_or_else PRIVATE nlohmann_json::nlohmann_json)
        add_test(NAME test_json_or_else COMMAND test_json_or_else)
    endif()

    # RapidJSON
    option(MUESLI_TEST_RAPIDJSON "Build RapidJSON backend tests" ON)
    if(MUESLI_TEST_RAPIDJSON)
        FetchContent_Declare(rapidjson
            GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
            GIT_TAG        24b5e7a8b27f42fa16b96fc70aade9106cf7102f
            SOURCE_SUBDIR  _do_not_build
        )
        FetchContent_MakeAvailable(rapidjson)

        add_muesli_test_exe(test_json_rapidjson tests/test_json_rapidjson.cpp)
        target_include_directories(test_json_rapidjson SYSTEM PRIVATE ${rapidjson_SOURCE_DIR}/include)
        add_test(NAME test_json_rapidjson COMMAND test_json_rapidjson)
    endif()

    # JsonCpp
    option(MUESLI_TEST_JSONCPP "Build JsonCpp backend tests" ON)
    if(MUESLI_TEST_JSONCPP)
        FetchContent_Declare(jsoncpp
            GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
            GIT_TAG        1.9.6
            GIT_SHALLOW    TRUE
        )
        set(JSONCPP_WITH_TESTS OFF CACHE BOOL "" FORCE)
        set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
        set(BUILD_OBJECT_LIBS OFF CACHE BOOL "" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(jsoncpp)

        add_muesli_test_exe(test_json_jsoncpp tests/test_json_jsoncpp.cpp)
        target_link_libraries(test_json_jsoncpp PRIVATE jsoncpp_static)
        add_test(NAME test_json_jsoncpp COMMAND test_json_jsoncpp)
    endif()

    # Boost.JSON
    option(MUESLI_TEST_BOOST_JSON "Build Boost.JSON backend tests" ON)
    if(MUESLI_TEST_BOOST_JSON)
        set(BOOST_INCLUDE_LIBRARIES json throw_exception)
        set(BOOST_ENABLE_CMAKE ON)
        FetchContent_Declare(Boost
            GIT_REPOSITORY https://github.com/boostorg/boost.git
            GIT_TAG        boost-1.87.0
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(Boost)

        # Boost::json is a built target (boost_json). On MSVC, compile it with /EHsc
        # so BOOST_NO_EXCEPTIONS is not inferred from missing unwind semantics.
        if(MSVC AND TARGET boost_json)
            target_compile_options(boost_json PRIVATE /EHsc)
        endif()

        add_muesli_test_exe(test_json_boost tests/test_json_boost.cpp)
        target_link_libraries(test_json_boost PRIVATE Boost::json Boost::throw_exception)
        if(MSVC)
            target_compile_options(test_json_boost PRIVATE /EHsc)
        else()
            target_compile_options(test_json_boost PRIVATE -fexceptions)
        endif()
        add_test(NAME test_json_boost COMMAND test_json_boost)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32 AND MUESLI_TEST_TARGETS)
        get_filename_component(MUESLI_CLANG_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        set_tests_properties(${MUESLI_TEST_TARGETS} PROPERTIES
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${MUESLI_CLANG_RUNTIME_DIR}"
        )
    endif()

    # Convenience target: run tests in parallel to reduce total local wall-time.
    set(MUESLI_CTEST_JOBS "0" CACHE STRING "Number of jobs for the 'check' test target (0 = ctest default)")
    if(MUESLI_CTEST_JOBS STREQUAL "0")
        add_custom_target(check
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> --output-on-failure
            DEPENDS ${MUESLI_TEST_TARGETS}
            USES_TERMINAL
        )
    else()
        add_custom_target(check
            COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIG> -j ${MUESLI_CTEST_JOBS} --output-on-failure
            DEPENDS ${MUESLI_TEST_TARGETS}
            USES_TERMINAL
        )
    endif()
endif()

if(BUILD_EXAMPLES)
    add_executable(example_01 examples/example_01.cpp)
    target_link_libraries(example_01 PRIVATE muesli)

    include(FetchContent)
    if(NOT TARGET nlohmann_json::nlohmann_json)
        FetchContent_Declare(nlohmann_json
                GIT_REPOSITORY https://github.com/nlohmann/json.git
                GIT_TAG        v3.11.3
                GIT_SHALLOW    TRUE
        )
        set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(nlohmann_json)
    endif()

    add_executable(example_02 examples/example_02.cpp)
    target_link_libraries(example_02 PRIVATE muesli nlohmann_json::nlohmann_json)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include)
install(TARGETS muesli EXPORT muesliTargets)
install(EXPORT muesliTargets
        FILE muesliTargets.cmake
        NAMESPACE muesli::
        DESTINATION lib/cmake/muesli)
