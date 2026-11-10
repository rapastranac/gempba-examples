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
        set_tests_properties(${p_name} PROPERTIES
                ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:gempba::gempba>"
        )
    endif ()
endfunction()

add_executable(multithreading_leaf_visitor multithreading_leaf_visitor.cpp)
target_link_libraries(multithreading_leaf_visitor PRIVATE gempba::gempba)
target_compile_options(multithreading_leaf_visitor PRIVATE -Wall -Wextra -Wpedantic)

foreach (v_threads 2 8 16)
    gempba_examples_add_stress_test(stress_multithreading_${v_threads}_threads multithreading
            $<TARGET_FILE:multithreading_leaf_visitor> ${v_threads} ${v_depth})
endforeach ()
