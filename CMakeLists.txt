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
option(GEMPBA_MULTIPROCESSING "Build the multiprocessing example tree (requires a multiprocessing-enabled gempba install)" ON)

# Consume gempba via find_package; this is the whole point of hosting the
# example tree in a sibling repo — every PR exercises the install/find_package
# surface, not the in-tree source tree.
find_package(gempba CONFIG REQUIRED)

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
