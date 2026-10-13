# SPDX-License-Identifier: GPL-3.0-only
#
# Oboe (https://github.com/google/oboe, Apache-2.0): low latency audio for Android.
# Built from source with the app toolchain. Set FETCHCONTENT_SOURCE_DIR_OBOE to use a local checkout.

include(FetchContent)

set(OBOE_VERSION "1.11.0" CACHE STRING "Oboe version (git tag)")

FetchContent_Declare(oboe
    GIT_REPOSITORY https://github.com/google/oboe.git
    GIT_TAG ${OBOE_VERSION}
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(oboe)
