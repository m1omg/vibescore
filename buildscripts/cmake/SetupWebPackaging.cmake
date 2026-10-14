# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore web app packaging: next to the wasm and the scripts that Qt generates (in CMAKE_RUNTIME_OUTPUT_DIRECTORY,
# see SetupBuildEnvironment.cmake), the page (index.html), its loader (vibescore-web.js), the web app manifest,
# the icon, and the application data packed into share.data (see buildscripts/web/pack_share.py). The page is
# stamped with the build (see buildscripts/web/stamp_page.py).

function(vs_setup_web_packaging target)
    set(web_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(stage_dir "${CMAKE_BINARY_DIR}/web-share-stage")
    set(web_src "${PROJECT_SOURCE_DIR}/buildscripts/web")

    # The page loads the app through this name (window.VibeScore_entry)
    set_target_properties(${target} PROPERTIES QT_WASM_EXPORT_NAME "${VS_BRAND_NAME_MACHINE}_entry")
    # Our page instead of Qt's (Qt still writes ${VS_BRAND_NAME_MACHINE}.html; it is not used)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    # Translations (downloaded as language packs), the sound font (the page downloads it after the app started)
    # and the test scripts are not in share.data
    add_custom_target(vs_web_package ALL
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${stage_dir}"
        COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${stage_dir}"
        COMMAND ${Python3_EXECUTABLE} "${web_src}/pack_share.py" "${stage_dir}/share" "${web_dir}"
                --exclude locale --exclude sound --exclude autobotscripts --exclude extensions --exclude plugins
        COMMAND ${CMAKE_COMMAND} -E copy "${web_src}/index.html" "${web_src}/vibescore-web.js" "${web_src}/manifest.json"
                "${web_dir}/"
        COMMAND ${Python3_EXECUTABLE} "${web_src}/stamp_page.py" "${web_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy
                "${PROJECT_SOURCE_DIR}/src/appshell/qml/MuseScore/AppShell/resources/vibescore_logo.svg"
                "${web_dir}/vibescore.svg"
        # Downloaded by the page after the app started
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${stage_dir}/share/sound" "${web_dir}/sound"
        COMMENT "Packaging the web app in ${web_dir}"
        VERBATIM
    )
    add_dependencies(vs_web_package ${target})
endfunction()
