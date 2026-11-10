cmake_minimum_required(VERSION 3.28)

project(gempba_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

option(GEMPBA_EXAMPLES_MPI "Build the multiprocessing examples too (requires gempba's MPI component, libgempba-mpi-dev)" ON)

if (GEMPBA_EXAMPLES_MPI)
    set(_gempba_components mpi)
else ()
    set(_gempba_components)
endif ()

# Local-IDE escape hatch: if find_package can't locate an installed gempba but
# a sibling source tree exists at ../gempba, fold it in via add_subdirectory.
# CLion users get a one-step build/debug; CI and packaging are unaffected.
find_package(gempba CONFIG QUIET COMPONENTS ${_gempba_components})
if (NOT gempba_FOUND)
    set(_gempba_sibling "${CMAKE_CURRENT_SOURCE_DIR}/../gempba")
    if (EXISTS "${_gempba_sibling}/CMakeLists.txt")
        message(STATUS "Installed gempba not found; using sibling source at ${_gempba_sibling}")
        set(GEMPBA_BUILD_MPI ${GEMPBA_EXAMPLES_MPI} CACHE BOOL "" FORCE)
        add_subdirectory("${_gempba_sibling}" "${CMAKE_BINARY_DIR}/_deps/gempba")
    else ()
        find_package(gempba CONFIG REQUIRED COMPONENTS ${_gempba_components})
    endif ()
    unset(_gempba_sibling)
endif ()
unset(_gempba_components)

# Static linking of Boost — examples use boost::serialization for task packets.
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS serialization fiber REQUIRED)

# The examples log with spdlog and format with fmt themselves; gempba keeps both private.
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_subdirectory(external)

# The multithreading examples need gempba's core, the multiprocessing ones its MPI component too.
add_subdirectory(multithreading)
if (GEMPBA_EXAMPLES_MPI)
    add_subdirectory(multiprocessing)
endif ()
