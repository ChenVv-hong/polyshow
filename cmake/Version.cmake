# Version management for PolyShow.
# Centralizes the application version and, when available, the current Git commit hash.

set(POLYSHOW_VERSION_MAJOR 0)
set(POLYSHOW_VERSION_MINOR 1)
set(POLYSHOW_VERSION_PATCH 0)

set(POLYSHOW_VERSION_STRING
    "${POLYSHOW_VERSION_MAJOR}.${POLYSHOW_VERSION_MINOR}.${POLYSHOW_VERSION_PATCH}"
)

# Try to discover the current short Git commit hash. Silently fall back to
# "unknown" when Git is unavailable or the source tree is not a repository.
set(POLYSHOW_GIT_COMMIT_HASH "unknown")

find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE git_rev_parse_result
        OUTPUT_VARIABLE git_rev_parse_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(git_rev_parse_result EQUAL 0 AND git_rev_parse_output)
        set(POLYSHOW_GIT_COMMIT_HASH "${git_rev_parse_output}")
    endif()
endif()

message(STATUS "PolyShow version: ${POLYSHOW_VERSION_STRING} (${POLYSHOW_GIT_COMMIT_HASH})")
