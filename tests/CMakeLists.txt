# Synthetic stress tests: each program walks a full binary tree, checks its own answers and exits non-zero when one is
# wrong. TIMEOUT turns a hang into a failure. RUN_SERIAL keeps the runs from competing for the cores they oversubscribe.
set(v_depth 16)

function(gempba_examples_add_stress_test p_name p_labels)
    add_test(NAME ${p_name} COMMAND ${ARGN})
    set_tests_properties(${p_name} PROPERTIES
            TIMEOUT 60
            RUN_SERIAL TRUE
            LABELS "stress;${p_labels}"
    )
    # Windows finds DLLs on PATH only, and gempba's are in its own bin directory.
    if (WIN32)
        set(v_path "PATH=path_list_prepend:$<TARGET_FILE_DIR:gempba::gempba>")
        if (TARGET gempba::mpi)
            list(APPEND v_path "PATH=path_list_prepend:$<TARGET_FILE_DIR:gempba::mpi>")
        endif ()
        set_tests_properties(${p_name} PROPERTIES ENVIRONMENT_MODIFICATION "${v_path}")
    endif ()
endfunction()

add_executable(multithreading_leaf_visitor multithreading_leaf_visitor.cpp)
target_link_libraries(multithreading_leaf_visitor PRIVATE gempba::gempba)
target_compile_options(multithreading_leaf_visitor PRIVATE -Wall -Wextra -Wpedantic)

foreach (v_threads 2 8 16)
    gempba_examples_add_stress_test(stress_multithreading_${v_threads}_threads multithreading
            $<TARGET_FILE:multithreading_leaf_visitor> ${v_threads} ${v_depth})
endforeach ()

if (GEMPBA_EXAMPLES_MPI)
    find_package(MPI REQUIRED COMPONENTS CXX)
    # FindMPI looks for MS-MPI's mpiexec on PATH only, which an MSYS2 shell leaves out. MS-MPI's installer sets MSMPI_BIN.
    if (NOT MPIEXEC_EXECUTABLE AND DEFINED ENV{MSMPI_BIN})
        file(TO_CMAKE_PATH "$ENV{MSMPI_BIN}" v_msmpi_bin)
        find_program(MPIEXEC_EXECUTABLE NAMES mpiexec HINTS "${v_msmpi_bin}")
    endif ()
    if (NOT MPIEXEC_EXECUTABLE)
        message(FATAL_ERROR "The MPI stress tests need mpiexec, which FindMPI did not find. Set MPIEXEC_EXECUTABLE, or pass -DGEMPBA_EXAMPLES_MPI=OFF.")
    endif ()

    add_executable(multiprocessing_leaf_visitor multiprocessing_leaf_visitor.cpp)
    target_link_libraries(multiprocessing_leaf_visitor PRIVATE gempba::mpi)
    target_compile_options(multiprocessing_leaf_visitor PRIVATE -Wall -Wextra -Wpedantic)

    # Open MPI refuses more ranks than cores unless it may oversubscribe them. MS-MPI has neither the limit nor the flag.
    set(v_mpiexec_preflags ${MPIEXEC_PREFLAGS})
    execute_process(COMMAND ${MPIEXEC_EXECUTABLE} --version OUTPUT_VARIABLE v_mpiexec_version ERROR_QUIET TIMEOUT 30)
    if (v_mpiexec_version MATCHES "Open MPI|OpenRTE" AND NOT "--oversubscribe" IN_LIST v_mpiexec_preflags)
        list(APPEND v_mpiexec_preflags --oversubscribe)
    endif ()

    foreach (v_ranks 2 4 8)
        gempba_examples_add_stress_test(stress_multiprocessing_${v_ranks}_ranks mpi
                ${MPIEXEC_EXECUTABLE} ${MPIEXEC_NUMPROC_FLAG} ${v_ranks} ${v_mpiexec_preflags}
                $<TARGET_FILE:multiprocessing_leaf_visitor> ${MPIEXEC_POSTFLAGS} 2 ${v_depth})
    endforeach ()
endif ()
