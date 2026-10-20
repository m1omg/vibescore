# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore: services for mobile platforms (Android, iOS), also used by the web app

muse_create_module(mobile)

target_sources(mobile PRIVATE
    mobilemodule.cpp
    mobilemodule.h

    internal/backnavigationcontroller.cpp
    internal/backnavigationcontroller.h
    internal/bluetoothmidicontroller.cpp
    internal/bluetoothmidicontroller.h
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
        internal/platform/android/androidbluetoothmidi.cpp
        internal/platform/android/androidbluetoothmidi.h
        internal/platform/android/androidincomingfiles.cpp
        internal/platform/android/androidincomingfiles.h
        internal/platform/android/androidprint.cpp
        internal/platform/android/androidprint.h
        internal/platform/android/androidshare.cpp
        internal/platform/android/androidshare.h
    )
endif()

if (OS_IS_IOS)
    target_sources(mobile PRIVATE
        internal/platform/ios/iosplatform.h
        internal/platform/ios/iosplatform.mm
    )

    set_source_files_properties(
        internal/platform/ios/iosplatform.mm
        PROPERTIES
        SKIP_UNITY_BUILD_INCLUSION ON
        SKIP_PRECOMPILE_HEADERS ON
    )

    find_library(UIKit NAMES UIKit)
    find_library(CoreAudioKit NAMES CoreAudioKit)
    target_link_libraries(mobile PRIVATE ${UIKit} ${CoreAudioKit})
endif()

if (OS_IS_WASM)
    target_sources(mobile PRIVATE
        internal/platform/web/webplatform.cpp
        internal/platform/web/webplatform.h
    )
    # QWasmSuspendResumeControl (see web::fixNestedEventLoops), QWindowSystemInterface (see web::watchPageVisibility)
    find_package(Qt6 REQUIRED COMPONENTS CorePrivate GuiPrivate)
    target_link_libraries(mobile PRIVATE Qt::CorePrivate Qt::GuiPrivate)
endif()

add_subdirectory(qml/MuseScore/Mobile)
