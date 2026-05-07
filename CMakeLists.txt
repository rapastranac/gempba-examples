cmake_minimum_required(VERSION 3.28)

project(gempba_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Consume gempba via find_package; this is the whole point of hosting the
# example tree in a sibling repo — every PR exercises the install/find_package
# surface, not the in-tree source tree.
find_package(gempba CONFIG REQUIRED)

# Smoke-test target until gempba#85 merges and the multithreading/multiprocessing
# example subdirectories migrate over here per gempba#86.
add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE gempba::gempba)

set_target_properties(smoke_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
)
