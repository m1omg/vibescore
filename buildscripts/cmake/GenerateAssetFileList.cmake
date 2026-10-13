# SPDX-License-Identifier: GPL-3.0-only
#
# Writes the list of files below DIR (relative paths, one per line) to OUTPUT.
# The Android asset manager can not list subdirectories, so the app reads this list
# to extract its data (see src/framework/global/internal/platform/android/androidappdata.cpp).
#
# Usage: cmake -DDIR=<dir> -DOUTPUT=<file> -P GenerateAssetFileList.cmake

file(GLOB_RECURSE files RELATIVE "${DIR}" "${DIR}/*")
list(FILTER files EXCLUDE REGEX "^vibescore-files\\.txt$")
list(SORT files)
string(REPLACE ";" "\n" content "${files}")
file(WRITE "${OUTPUT}" "${content}\n")
list(LENGTH files count)
message(STATUS "Asset file list: ${count} files")
