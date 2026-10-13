# SPDX-License-Identifier: GPL-3.0-only
#
# Android packaging for the app-mobile configuration (VibeScore).
#
# Generates the Android package source directory (manifest, gradle file, resources, Java sources)
# from buildscripts/packaging/Android and stages the application data (share/) into the APK assets.
# androiddeployqt copies this directory over Qt's templates when the APK/AAB is built.

set(VS_ANDROID_MIN_SDK_VERSION 28 CACHE STRING "Android minSdkVersion")
set(VS_ANDROID_TARGET_SDK_VERSION 35 CACHE STRING "Android targetSdkVersion")

function(vs_setup_android_packaging target)
    set(template_dir "${PROJECT_SOURCE_DIR}/buildscripts/packaging/Android")
    set(package_dir "${CMAKE_BINARY_DIR}/android-package-source")
    set(stage_dir "${CMAKE_BINARY_DIR}/android-share-stage")

    file(MAKE_DIRECTORY "${package_dir}")
    configure_file("${template_dir}/AndroidManifest.xml.in" "${package_dir}/AndroidManifest.xml" @ONLY)
    configure_file("${template_dir}/build.gradle" "${package_dir}/build.gradle" COPYONLY)

    file(GLOB_RECURSE template_files CONFIGURE_DEPENDS
        RELATIVE "${template_dir}"
        "${template_dir}/res/*"
        "${template_dir}/src/*")
    foreach(f ${template_files})
        configure_file("${template_dir}/${f}" "${package_dir}/${f}" COPYONLY)
    endforeach()

    set_target_properties(${target} PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${package_dir}"
        QT_ANDROID_PACKAGE_NAME "${VS_BRAND_APP_ID}"
        QT_ANDROID_APP_NAME "${VS_BRAND_NAME}"
        QT_ANDROID_APP_ICON "@mipmap/ic_launcher"
        QT_ANDROID_MIN_SDK_VERSION ${VS_ANDROID_MIN_SDK_VERSION}
        QT_ANDROID_TARGET_SDK_VERSION ${VS_ANDROID_TARGET_SDK_VERSION}
        QT_ANDROID_COMPILE_SDK_VERSION ${VS_ANDROID_TARGET_SDK_VERSION}
        QT_ANDROID_VERSION_CODE ${VS_BRAND_VERSION_CODE}
        QT_ANDROID_VERSION_NAME "${VS_BRAND_VERSION}"
    )

    # Release signing: androiddeployqt reads QT_ANDROID_KEYSTORE_{PATH,ALIAS,STORE_PASS,KEY_PASS} from the environment
    # (Qt reads these as variables in the target's directory scope when it finalizes the target)
    if (DEFINED ENV{QT_ANDROID_KEYSTORE_PATH})
        set(QT_ANDROID_SIGN_APK ON PARENT_SCOPE)
        set(QT_ANDROID_SIGN_AAB ON PARENT_SCOPE)
    endif()

    # QML files are spread over all modules; androiddeployqt scans them to deploy the Qt QML plugins they import
    set_property(TARGET ${target} APPEND PROPERTY QT_QML_ROOT_PATH "${PROJECT_SOURCE_DIR}/src")

    # HTTPS (Learn, language packs, plugins): Qt's TLS backend needs OpenSSL
    include(SetupAndroidOpenSSL)
    vs_add_android_openssl(${target})

    # Native libraries must be aligned to 16 KB pages for Android 15+ devices
    target_link_options(${target} PRIVATE "-Wl,-z,max-page-size=16384")

    # Stage share/ (templates, instruments, soundfont, translations, ...) into the APK assets.
    # Everything is installed into a staging prefix and only share/ is copied into the package.
    add_custom_target(vs_stage_android_assets
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${stage_dir}" "${package_dir}/assets/share"
        COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${stage_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${stage_dir}/share" "${package_dir}/assets/share"
        COMMAND ${CMAKE_COMMAND} -DDIR=${package_dir}/assets/share -DOUTPUT=${package_dir}/assets/share/vibescore-files.txt
                -P ${PROJECT_SOURCE_DIR}/buildscripts/cmake/GenerateAssetFileList.cmake
        COMMENT "Staging application data into the Android package assets"
        VERBATIM
    )
    add_dependencies(vs_stage_android_assets ${target})
    if (TARGET translations)
        add_dependencies(vs_stage_android_assets translations)
    endif()

    # The APK is assembled by <target>_make_apk (created by qt_finalize_target)
    cmake_language(EVAL CODE "
        cmake_language(DEFER DIRECTORY [[${CMAKE_SOURCE_DIR}]] CALL vs_android_hook_assets [[${target}]])
    ")
endfunction()

function(vs_android_hook_assets target)
    if (TARGET ${target}_make_apk)
        add_dependencies(${target}_make_apk vs_stage_android_assets)
    endif()
    if (TARGET ${target}_make_aab)
        add_dependencies(${target}_make_aab vs_stage_android_assets)
    endif()
endfunction()
