# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore: services for mobile platforms (Android, iOS)

muse_create_module(mobile)

target_sources(mobile PRIVATE
    mobilemodule.cpp
    mobilemodule.h

    internal/backnavigationcontroller.cpp
    internal/backnavigationcontroller.h
    internal/incomingfilescontroller.cpp
    internal/incomingfilescontroller.h
    internal/mobileinteractive.cpp
    internal/mobileinteractive.h
    internal/mobilelifecyclecontroller.cpp
    internal/mobilelifecyclecontroller.h
    internal/mobileprintprovider.cpp
    internal/mobileprintprovider.h
    internal/mobilewindowsprovider.cpp
    internal/mobilewindowsprovider.h
    internal/scoreslibrary.cpp
    internal/scoreslibrary.h
)

target_link_libraries(mobile PRIVATE
    muse::uicomponents_qml
    Qt::Widgets
)

if (OS_IS_ANDROID)
    target_sources(mobile PRIVATE
        internal/platform/android/androidincomingfiles.cpp
        internal/platform/android/androidincomingfiles.h
        internal/platform/android/androidprint.cpp
        internal/platform/android/androidprint.h
        internal/platform/android/androidshare.cpp
        internal/platform/android/androidshare.h
    )
endif()

add_subdirectory(qml/MuseScore/Mobile)
