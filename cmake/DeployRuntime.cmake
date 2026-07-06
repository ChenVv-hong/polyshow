# Reusable runtime deployment helpers for executable targets.

include(GNUInstallDirs)

function(_deploy_runtime_find_windeployqt out_var)
    if(NOT WIN32)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(windeployqt_hints "")
    if(TARGET Qt6::qmake)
        get_target_property(qmake_location Qt6::qmake IMPORTED_LOCATION)
        if(qmake_location)
            get_filename_component(qt_bin_dir "${qmake_location}" DIRECTORY)
            list(APPEND windeployqt_hints "${qt_bin_dir}")
        endif()
    endif()

    foreach(prefix_path IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND windeployqt_hints "${prefix_path}/bin")
    endforeach()

    find_program(windeployqt_executable NAMES windeployqt HINTS ${windeployqt_hints})
    if(NOT windeployqt_executable)
        message(FATAL_ERROR "windeployqt was not found. Set CMAKE_PREFIX_PATH to a Qt install that contains bin/windeployqt.exe.")
    endif()

    set(${out_var} "${windeployqt_executable}" PARENT_SCOPE)
endfunction()

function(_deploy_runtime_add_runtime_copy target)
    cmake_parse_arguments(ARG "" "" "EXTRA_DLLS" ${ARGN})

    set(script_path "${CMAKE_CURRENT_BINARY_DIR}/deploy-runtime/$<CONFIG>/${target}-copy-runtime.cmake")
    file(GENERATE
        OUTPUT "${script_path}"
        CONTENT "
set(runtime_dlls \"$<TARGET_RUNTIME_DLLS:${target}>\")
set(extra_dlls \"${ARG_EXTRA_DLLS}\")
set(destination \"$<TARGET_FILE_DIR:${target}>\")

foreach(runtime_dll IN LISTS runtime_dlls extra_dlls)
    if(runtime_dll AND EXISTS \"\${runtime_dll}\")
        get_filename_component(runtime_dll_name \"\${runtime_dll}\" NAME)
        file(COPY_FILE \"\${runtime_dll}\" \"\${destination}/\${runtime_dll_name}\" ONLY_IF_DIFFERENT)
    endif()
endforeach()
"
    )

    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -P "${script_path}"
        VERBATIM
    )
endfunction()

function(_deploy_runtime_add_windeployqt target)
    cmake_parse_arguments(ARG "" "" "OPTIONS" ${ARGN})

    if(NOT WIN32)
        return()
    endif()

    _deploy_runtime_find_windeployqt(windeployqt_executable)
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND "${windeployqt_executable}"
                $<$<CONFIG:Debug>:--debug>
                $<$<NOT:$<CONFIG:Debug>>:--release>
                ${ARG_OPTIONS}
                "$<TARGET_FILE:${target}>"
        VERBATIM
        COMMAND_EXPAND_LISTS
    )
endfunction()

function(_deploy_runtime_add_qt_install target)
    cmake_parse_arguments(ARG "" "RUNTIME_DESTINATION" "OPTIONS" ${ARGN})

    if(NOT WIN32)
        if(COMMAND qt_generate_deploy_app_script)
            qt_generate_deploy_app_script(
                TARGET ${target}
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                DEPLOY_TOOL_OPTIONS ${ARG_OPTIONS}
            )
        elseif(COMMAND qt6_generate_deploy_app_script)
            qt6_generate_deploy_app_script(
                TARGET ${target}
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                DEPLOY_TOOL_OPTIONS ${ARG_OPTIONS}
            )
        else()
            message(FATAL_ERROR "Qt deploy app script command is unavailable. Find Qt6::Core before calling deploy_runtime_targets().")
        endif()

        install(SCRIPT "${deploy_script}")
        return()
    endif()

    _deploy_runtime_find_windeployqt(windeployqt_executable)
    install(CODE "
set(deploy_runtime_dir \"\${CMAKE_INSTALL_PREFIX}/${ARG_RUNTIME_DESTINATION}\")
execute_process(
    COMMAND \"${windeployqt_executable}\"
            $<$<CONFIG:Debug>:--debug>
            $<$<NOT:$<CONFIG:Debug>>:--release>
            ${ARG_OPTIONS}
            --dir \"\${deploy_runtime_dir}\"
            \"\${deploy_runtime_dir}/$<TARGET_FILE_NAME:${target}>\"
    RESULT_VARIABLE deploy_runtime_result
)
if(NOT deploy_runtime_result EQUAL 0)
    message(FATAL_ERROR \"windeployqt failed for ${target}: \${deploy_runtime_result}\")
endif()
")
endfunction()

function(deploy_runtime_targets)
    cmake_parse_arguments(
        ARG
        ""
        "RUNTIME_DESTINATION;LIBRARY_DESTINATION"
        "TARGETS;QT_TARGETS;EXTRA_DLLS;WINDEPLOYQT_OPTIONS"
        ${ARGN}
    )

    if(NOT ARG_TARGETS AND NOT ARG_QT_TARGETS)
        message(FATAL_ERROR "deploy_runtime_targets() requires TARGETS or QT_TARGETS.")
    endif()

    if(NOT ARG_RUNTIME_DESTINATION)
        set(ARG_RUNTIME_DESTINATION ".")
    endif()

    if(NOT ARG_LIBRARY_DESTINATION)
        set(ARG_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    endif()

    set(all_targets ${ARG_TARGETS} ${ARG_QT_TARGETS})
    list(REMOVE_DUPLICATES all_targets)

    foreach(target IN LISTS all_targets)
        if(NOT TARGET ${target})
            message(FATAL_ERROR "deploy_runtime_targets() was given an unknown target: ${target}")
        endif()

        _deploy_runtime_add_runtime_copy(${target} EXTRA_DLLS ${ARG_EXTRA_DLLS})
    endforeach()

    foreach(target IN LISTS ARG_QT_TARGETS)
        _deploy_runtime_add_windeployqt(${target} OPTIONS ${ARG_WINDEPLOYQT_OPTIONS})
    endforeach()

    set(runtime_dependency_set "${PROJECT_NAME}_runtime_dependencies")
    install(
        TARGETS ${all_targets}
        RUNTIME_DEPENDENCY_SET ${runtime_dependency_set}
        RUNTIME DESTINATION "${ARG_RUNTIME_DESTINATION}"
        LIBRARY DESTINATION "${ARG_LIBRARY_DESTINATION}"
        ARCHIVE DESTINATION "${ARG_LIBRARY_DESTINATION}"
    )

    if(ARG_EXTRA_DLLS)
        install(FILES ${ARG_EXTRA_DLLS} DESTINATION "${ARG_RUNTIME_DESTINATION}")
    endif()

    install(
        RUNTIME_DEPENDENCY_SET ${runtime_dependency_set}
        DESTINATION "${ARG_RUNTIME_DESTINATION}"
        PRE_EXCLUDE_REGEXES
            [[api-ms-.*]]
            [[ext-ms-.*]]
        POST_EXCLUDE_REGEXES
            [[.*[\\/]system32[\\/].*]]
            [[.*[\\/]System32[\\/].*]]
    )

    foreach(target IN LISTS ARG_QT_TARGETS)
        _deploy_runtime_add_qt_install(
            ${target}
            RUNTIME_DESTINATION "${ARG_RUNTIME_DESTINATION}"
            OPTIONS ${ARG_WINDEPLOYQT_OPTIONS}
        )
    endforeach()
endfunction()
