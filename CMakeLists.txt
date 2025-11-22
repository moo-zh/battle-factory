cmake_minimum_required(VERSION 3.20)
project(BattleFactory LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

# Battle engine library (platform-agnostic)
file(GLOB_RECURSE BATTLE_SOURCES
    "src/battle/*.cpp"
)

add_library(battle_engine STATIC ${BATTLE_SOURCES})
target_include_directories(battle_engine PUBLIC src/)

# Host build configuration
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # Enable testing
    enable_testing()

    # Fetch GTest if not found
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found, fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0  # Use a stable release
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    include(GoogleTest)

    # Test helpers library
    file(GLOB TEST_HELPER_SOURCES "test/host/helpers/*.cpp")
    add_library(test_helpers STATIC ${TEST_HELPER_SOURCES})
    target_include_directories(test_helpers PUBLIC src/ test/host/helpers/)
    target_link_libraries(test_helpers PUBLIC battle_engine)

    # Host tests (GTest-based)
    file(GLOB_RECURSE TEST_SOURCES "test/host/**/*.cpp")
    # Exclude helper sources (already in test_helpers library)
    list(FILTER TEST_SOURCES EXCLUDE REGEX "test/host/helpers/.*\\.cpp$")

    if(TEST_SOURCES)
        add_executable(unit_tests ${TEST_SOURCES})
        target_link_libraries(unit_tests PRIVATE battle_engine test_helpers GTest::GTest GTest::Main)
        target_include_directories(unit_tests PRIVATE
            src/
            test/host/helpers/
            test/host/  # For test_common.hpp
        )

        gtest_discover_tests(unit_tests)
    else()
        message(STATUS "No test sources found in test/host/")
    endif()
endif()
