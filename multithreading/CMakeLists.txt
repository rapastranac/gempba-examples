file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

foreach (EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
    set(TARGET_NAME multithreading_${EXAMPLE_NAME})

    add_executable(${TARGET_NAME} ${EXAMPLE_SOURCE})

    target_include_directories(${TARGET_NAME} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${Boost_INCLUDE_DIRS}
            ${ARGPARSE_INCLUDE_DIR}
    )

    target_link_libraries(${TARGET_NAME} PRIVATE
            gempba::gempba
            example_libs
            Boost::serialization
            Boost::fiber
    )

    set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    )

    # Windows: Copy transitively-linked runtime DLLs (libgempba, MSYS2 libgcc/libstdc++/libwinpthread, ...) next to the
    # exe so the loader finds them. Expands to empty on non-DLL platforms.
    if (WIN32)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
                        $<TARGET_FILE_DIR:${TARGET_NAME}>
                COMMAND_EXPAND_LISTS
        )
    endif ()

    target_compile_options(${TARGET_NAME} PRIVATE
            -Wall -Wextra -Wpedantic
            $<$<CONFIG:Debug>:-O0 -ggdb3>
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE
            $<$<CONFIG:Debug>:GEMPBA_DEBUG_COMMENTS>
    )
endforeach ()
