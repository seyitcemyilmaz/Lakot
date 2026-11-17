cmake_minimum_required(VERSION 3.16)

execute_process(
    COMMAND git -C "${SOURCE_DIR}" rev-parse --short=12 HEAD
    OUTPUT_VARIABLE tCommit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE tCommitResult)

if (NOT tCommitResult EQUAL 0 OR "${tCommit}" STREQUAL "")
    set(tCommit "unknown")
else()
    execute_process(
        COMMAND git -C "${SOURCE_DIR}" status --porcelain --untracked-files=no -- src cmake CMakeLists.txt
        OUTPUT_VARIABLE tDirty
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)

    if (NOT "${tDirty}" STREQUAL "")
        set(tCommit "${tCommit}-dirty")
    endif()
endif()

set(tContent "#define LAKOT_BUILD_COMMIT \"${tCommit}\"
")

if (EXISTS "${OUTPUT_FILE}")
    file(READ "${OUTPUT_FILE}" tExisting)
endif()

if (NOT "${tExisting}" STREQUAL "${tContent}")
    file(WRITE "${OUTPUT_FILE}" "${tContent}")
endif()
