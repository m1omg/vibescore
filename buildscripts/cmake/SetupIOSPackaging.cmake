# SPDX-License-Identifier: GPL-3.0-only
#
# iOS / iPadOS packaging for the app-mobile configuration (VibeScore).
# Builds a universal (iPhone + iPad) app bundle; the application data (share/) is copied into the bundle.
# Use the Xcode generator: qt-cmake -G Xcode -DMUSESCORE_BUILD_CONFIGURATION=app-mobile ...

set(VS_IOS_DEPLOYMENT_TARGET "17.0" CACHE STRING "Minimum iOS version")

function(vs_setup_ios_packaging target)
    set(template_dir "${PROJECT_SOURCE_DIR}/buildscripts/packaging/iOS")
    set(stage_dir "${CMAKE_BINARY_DIR}/ios-share-stage")

    # Brand identifiers are substituted here; the MACOSX_BUNDLE_* values are substituted by CMake afterwards
    configure_file("${template_dir}/Info.plist.in" "${CMAKE_BINARY_DIR}/ios/Info.plist.in" @ONLY)

    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/ios/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${VS_BRAND_APP_ID}"
        MACOSX_BUNDLE_BUNDLE_NAME "${VS_BRAND_NAME}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${VS_BRAND_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${VS_BRAND_VERSION_CODE}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${VS_BRAND_APP_ID}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${VS_IOS_DEPLOYMENT_TARGET}"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        QT_IOS_LAUNCH_SCREEN "${template_dir}/LaunchScreen.storyboard"
    )

    target_sources(${target} PRIVATE "${template_dir}/Assets.xcassets")
    set_source_files_properties("${template_dir}/Assets.xcassets" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # Translations are installed as part of share/
    if (TARGET translations)
        add_dependencies(${target} translations)
    endif()

    # Copy share/ (templates, instruments, styles, SoundFont, translations, ...) into the bundle
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DCONFIG=$<CONFIG>
            -DSTAGE_DIR=${stage_dir}
            -DBUNDLE_DIR=$<TARGET_BUNDLE_DIR:${target}>
            -P ${PROJECT_SOURCE_DIR}/buildscripts/cmake/CopyShareIntoBundle.cmake
        COMMENT "Copying application data into the app bundle"
        VERBATIM
    )
endfunction()
