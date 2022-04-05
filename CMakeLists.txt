cmake_minimum_required(VERSION 3.19)
project("SymbolGenerator")


# Prevent concurrent CMake runs.
file(LOCK ${CMAKE_SOURCE_DIR}/cmake.lock)


# Allow loading of scripts from cmake folder.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
# Store builds in out folder.
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/out")
# Allow including from the root directory.
include_directories(${CMAKE_SOURCE_DIR})


# Output to out directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/bin)


# Force C++20. (Note this must be done manually with MSVC, see cmake/profiles/msvc.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Install Dependencies
include(run_conan)
select_conan_profile(conan_profile)
run_conan(${conan_profile})

# Enable compiler-specific warnings
include(compiler_profile)
set_compiler_profile()

# Copy shared libraries from conan folder to binary folder.
if (EXISTS ${CMAKE_SOURCE_DIR}/out/conan/bin)
    add_custom_target(
        COPY_SHARED_LIBS ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/out/conan/bin
            ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# Add subprojects.
add_subdirectory(SymbolGenerator)