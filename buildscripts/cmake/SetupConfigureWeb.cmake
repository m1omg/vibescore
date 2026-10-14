# SPDX-License-Identifier: GPL-3.0-only
#
# CONFIGURE: Web App (VibeScore in the browser) - MUSESCORE_BUILD_CONFIGURATION=app-web
#
# The web app is the mobile app (the same shell, single window, touch-ready) built with Qt for WebAssembly
# (single thread), with the parts of the mobile app that a browser does not have turned off.

include(SetupConfigureMobile)

set(MUE_CONFIGURATION_IS_APPWEB ON)
set(MUSE_CONFIGURATION_IS_WEB ON)

# Single thread: no worker thread, no QtConcurrent, no processes
set(MUSE_THREADS_SUPPORT OFF)
set(QT_ADD_LINGUISTTOOLS OFF)
set(QT_ADD_CONCURRENT OFF)
set(QT_CONCURRENT_SUPPORTED OFF)
set(QT_QPROCESS_SUPPORTED OFF)

# The audio engine runs in the driver callback (no worker thread)
set(MUSE_MODULE_AUDIO_WORKMODE 2)

# Not available in a browser (yet)
set(MUSE_MODULE_MIDI OFF)           # Web MIDI: later
set(MUSE_MODULE_MIDIREMOTE OFF)
set(MUSE_MODULE_NETWORK OFF)        # language packs and Learn need the network module
set(MUSE_MODULE_LANGUAGES OFF)
set(MUSE_MODULE_LEARN OFF)
set(MUSE_MODULE_EXTENSIONS OFF)
set(MUSE_MODULE_ACCESSIBILITY OFF)
set(MUE_ENABLE_CONSOLEAPP OFF)
set(MUE_BUILD_CONVERTER_MODULE OFF)

# The sound font is installed (staged) like on mobile; the page downloads it after the app started
# (see SetupWebPackaging.cmake)
