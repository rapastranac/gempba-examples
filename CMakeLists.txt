cmake_minimum_required(VERSION 3.28)

project(gempba_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

# Pick the parallelism mode at configure time. Must match the mode the
# installed gempba was built with — gempba ships <gempba/config.h> with the
# baked-in value, but each example target also needs to know at configure time
# which subdirectory to add.
option(GEMPBA_MULTIPROCESSING "Build the multiprocessing example tree (requires a multiprocessing-enabled gempba install)" OFF)

# Consume gempba via find_package; this is the whole point of hosting the
# example tree in a sibling repo — every PR exercises the install/find_package
# surface, not the in-tree source tree.
#
# Local-IDE escape hatch: if find_package can't locate an installed gempba but
# a sibling source tree exists at ../gempba, fold it in via add_subdirectory.
# CLion users get a one-step build/debug; CI and packaging are unaffected.
find_package(gempba CONFIG QUIET)
if (NOT gempba_FOUND)
    set(_gempba_sibling "${CMAKE_CURRENT_SOURCE_DIR}/../gempba")
    if (EXISTS "${_gempba_sibling}/CMakeLists.txt")
        message(STATUS "Installed gempba not found; using sibling source at ${_gempba_sibling}")
        # gempba's root CMakeLists picks subproject defaults (TESTS=OFF,
        # DEV_MODE=OFF) automatically. Mirror our MULTIPROCESSING choice so
        # the sibling matches what the example tree expects.
        set(GEMPBA_MULTIPROCESSING ${GEMPBA_MULTIPROCESSING} CACHE BOOL "" FORCE)
        add_subdirectory("${_gempba_sibling}" "${CMAKE_BINARY_DIR}/_deps/gempba")
    else ()
        find_package(gempba CONFIG REQUIRED)
    endif ()
    unset(_gempba_sibling)
endif ()

# Static linking of Boost — examples use boost::serialization for task packets.
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost COMPONENTS serialization fiber REQUIRED)

add_subdirectory(external)

# The two example trees exercise mutually exclusive APIs: in a consumer build
# the inactive namespace is hidden, so multithreading/ examples only compile
# against an MP=OFF gempba and multiprocessing/ examples only compile against
# an MP=ON gempba. Pick the tree that matches the gempba install.
if (GEMPBA_MULTIPROCESSING)
    add_subdirectory(multiprocessing)
else ()
    add_subdirectory(multithreading)
endif ()
