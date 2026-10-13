# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore branding.
#
# VibeScore is an independent fork of MuseScore Studio for mobile devices and the web.
# It is not affiliated with or endorsed by Muse Group / MuseScore Limited.
#
# This file is the single source of truth for everything that identifies the app:
# its name, identifiers, URLs and the identity reported to the framework.
# To rebrand, change the values below; nothing else in the tree should hard-code them.

set(VS_BRAND_NAME "VibeScore")                       # Human readable name
set(VS_BRAND_NAME_MACHINE "VibeScore")               # No spaces; used for paths, executables, settings
set(VS_BRAND_APP_ID "io.github.m1omg.vibescore")     # Android package name / iOS bundle identifier
set(VS_BRAND_URL_SCHEME "vibescore")                 # Custom URL scheme
set(VS_BRAND_ORGANIZATION "VibeScore")               # Used for settings locations
set(VS_BRAND_DOMAIN "m1omg.github.io")
set(VS_BRAND_SOURCE_URL "https://github.com/m1omg/vibescore")
set(VS_BRAND_UPSTREAM_NAME "MuseScore Studio")
set(VS_BRAND_UPSTREAM_URL "https://github.com/musescore/MuseScore")

# Version of VibeScore itself (independent of the upstream MuseScore version it is based on)
set(VS_BRAND_VERSION_MAJOR 0)
set(VS_BRAND_VERSION_MINOR 1)
set(VS_BRAND_VERSION_PATCH 0)
set(VS_BRAND_VERSION "${VS_BRAND_VERSION_MAJOR}.${VS_BRAND_VERSION_MINOR}.${VS_BRAND_VERSION_PATCH}")

# Android versionCode must grow monotonically: MMmmppbb (bb = build number modulo 100)
math(EXPR VS_BRAND_VERSION_CODE
    "${VS_BRAND_VERSION_MAJOR} * 1000000 + ${VS_BRAND_VERSION_MINOR} * 10000 + ${VS_BRAND_VERSION_PATCH} * 100 + (${CMAKE_BUILD_NUMBER} % 100)")

# The identity the framework reports (window titles, About, user agent, ...)
set(MUSE_APP_NAME_HUMAN_READABLE "${VS_BRAND_NAME}")
set(MUSE_APP_NAME_MACHINE_READABLE "${VS_BRAND_NAME_MACHINE}")
set(MUSE_APP_NAME_HUMAN_READABLE_COMPAT "${VS_BRAND_NAME}")
set(MUSE_APP_NAME_MACHINE_READABLE_COMPAT "${VS_BRAND_NAME_MACHINE}")
set(MUSE_APP_GUI_IDENTIFIER "${VS_BRAND_APP_ID}")
set(MUSE_APP_TITLE "${VS_BRAND_NAME}")
set(MUSE_APP_NAME "${VS_BRAND_NAME_MACHINE}")
set(MUSE_APP_TITLE_VERSION "${MUSE_APP_TITLE}")
set(MUSE_APP_NAME_VERSION "${MUSE_APP_NAME}")
