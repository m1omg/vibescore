# SPDX-License-Identifier: GPL-3.0-only
#
# CONFIGURE: Mobile App (VibeScore) - MUSESCORE_BUILD_CONFIGURATION=app-mobile
#
# Used for Android and iOS builds, and also for desktop builds of the mobile
# configuration (a fast development loop for the touch UI on the host).

set(MUE_CONFIGURATION_IS_APPMOBILE ON)
set(MUSE_CONFIGURATION_IS_MOBILE ON)
set(MUE_GENERAL_APP ON)

include(MobileBranding)

set(MUSE_MODULE_AUDIO_WORKMODE 3) # driver callback + worker rpc

if (OS_IS_IOS)
    # QProcess does not exist on iOS
    set(QT_QPROCESS_SUPPORTED OFF)
endif()

# Single window, touch-first UI
set(MUSE_MODULE_UI_SYSTEMDRAG_SUPPORTED OFF)
set(MUE_ENABLE_SPLASHSCREEN OFF)

# Unit tests are built with the utest configuration on the host
set(MUSE_ENABLE_UNIT_TESTS OFF)

# Services operated by Muse Group are not used by VibeScore
set(MUSE_MODULE_CLOUD OFF)
set(MUSE_MODULE_UPDATE OFF)
set(MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT OFF)
set(MUE_BUILD_MUSESOUNDS_MODULE OFF)

# Not available on mobile platforms
set(MUSE_MODULE_MUSESAMPLER OFF) # proprietary library that is downloaded and loaded at runtime
set(MUSE_MODULE_AUDIOPLUGINS OFF)
set(MUSE_MODULE_VST OFF)
set(MUSE_MODULE_AUDIO_JACK OFF)
set(MUSE_MODULE_AUDIO_PIPEWIRE OFF)
set(MUSE_MODULE_MULTIWINDOWS OFF)
# The media module stays on (its stub is not maintained); it finds no FFmpeg on mobile
set(MUE_BUILD_IMPEXP_VIDEOEXPORT_MODULE OFF)
set(MUE_BUILD_MACOS_INTEGRATION OFF)

# AAC export uses fdk-aac, whose license is not compatible with the GPL
set(MUSE_MODULE_AUDIO_EXPORT_AAC OFF)

# Replaced by mobile implementations (PDF + native print dialog)
set(MUE_BUILD_PRINT_MODULE OFF)

# Braille tables are read with fopen from the app data directory; enabled later
set(MUE_BUILD_BRAILLE_MODULE OFF)

# The soundfont is in the source tree; never modify the tree during configure
set(MUE_DOWNLOAD_SOUNDFONT OFF)
set(MUE_INSTALL_SOUNDFONT ON)
