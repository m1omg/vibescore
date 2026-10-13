# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore: QML of the mobile module

muse_create_qml_module(mobile_qml FOR mobile)

qt_add_qml_module(mobile_qml
    URI MuseScore.Mobile
    VERSION 1.0
    QML_FILES
        SaveToLibraryDialog.qml
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
)

fixup_qml_module_dependencies(mobile_qml)
