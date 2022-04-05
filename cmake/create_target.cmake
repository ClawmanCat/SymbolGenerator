# Creates a target in the current directory with the given name and type (EXECUTABLE, INTERFACE, etc.).
# A list of targets can be provided as variadic parameters to pass a list of dependencies for the target.
function(create_target name type major minor patch)
    file(GLOB_RECURSE sources CONFIGURE DEPENDS LIST_DIRECTORIES false "*.cpp" "*.hpp")
    create_target_from_sources(${name} ${type} ${major} ${minor} ${patch} "${sources}" ${ARGN})
endfunction()


# Equivalent to create_target, but a list of sources is explicitly provided.
function(create_target_from_sources name type major minor patch sources)
    # Create target of correct type and add sources.
    if (${type} STREQUAL "EXECUTABLE")
        add_executable(${name} ${sources})
    elseif (${type} STREQUAL "INTERFACE")
        add_library(${name} ${type})
        target_sources(${sources})
    else ()
        add_library(${name} ${type} ${sources})
    endif()


    # Add dependencies.
    target_link_libraries_system(${name} ${ARGN})


    # Specifying the linker language manually is required for header only libraries,
    # since the language cannot be deduced from the file extension.
    set_target_properties(${name} PROPERTIES LINKER_LANGUAGE CXX)


    # Add preprocessor definitions for the target version.
    string(TOUPPER ${name} name_capitalized)
    target_compile_definitions(${name} PUBLIC "${name_capitalized}_VERSION_MAJOR=${major}")
    target_compile_definitions(${name} PUBLIC "${name_capitalized}_VERSION_MINOR=${minor}")
    target_compile_definitions(${name} PUBLIC "${name_capitalized}_VERSION_PATCH=${patch}")
endfunction()