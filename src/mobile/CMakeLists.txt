# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore: services for mobile platforms (Android, iOS)

muse_create_module(mobile)

target_sources(mobile PRIVATE
    mobilemodule.cpp
    mobilemodule.h

    internal/incomingfilescontroller.cpp
    internal/incomingfilescontroller.h
)

if (OS_IS_ANDROID)
    target_sources(mobile PRIVATE
        internal/platform/android/androidincomingfiles.cpp
        internal/platform/android/androidincomingfiles.h
    )
endif()
