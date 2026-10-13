# SPDX-License-Identifier: GPL-3.0-only
#
# Copies the application data (share/) into the iOS app bundle after it is built.
# Usage: cmake -DBUILD_DIR=<dir> -DCONFIG=<config> -DSTAGE_DIR=<dir> -DBUNDLE_DIR=<fallback> -P CopyShareIntoBundle.cmake
#
# Under Xcode the bundle path contains build settings (e.g. $(EFFECTIVE_PLATFORM_NAME)) that are only
# known while building, so it is taken from the environment Xcode provides to build phases.

if (DEFINED ENV{TARGET_BUILD_DIR} AND DEFINED ENV{WRAPPER_NAME})
    set(bundle "$ENV{TARGET_BUILD_DIR}/$ENV{WRAPPER_NAME}")
else()
    set(bundle "${BUNDLE_DIR}")
endif()

if (NOT IS_DIRECTORY "${bundle}")
    message(FATAL_ERROR "App bundle not found: ${bundle}")
endif()

file(REMOVE_RECURSE "${STAGE_DIR}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --config "${CONFIG}" --prefix "${STAGE_DIR}"
    RESULT_VARIABLE result
    OUTPUT_QUIET
)
if (NOT result EQUAL 0)
    message(FATAL_ERROR "Failed to stage the application data (${result})")
endif()

file(REMOVE_RECURSE "${bundle}/share")
file(COPY "${STAGE_DIR}/share" DESTINATION "${bundle}")
message(STATUS "Copied application data into ${bundle}/share")
