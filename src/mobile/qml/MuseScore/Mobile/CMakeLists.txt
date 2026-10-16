# SPDX-License-Identifier: GPL-3.0-only
#
# VibeScore: QML of the mobile module (the mobile shell and dialogs)

muse_create_qml_module(mobile_qml FOR mobile)

qt_add_qml_module(mobile_qml
    URI MuseScore.Mobile
    VERSION 1.0
    SOURCES
        mobileshellview.cpp
        mobileshellview.h
        touchmodifiersmodel.cpp
        touchmodifiersmodel.h
    QML_FILES
        AppMenuDrawer.qml
        BusyDialog.qml
        Main.qml
        MobileHomePage.qml
        MobileNotationPage.qml
        MobileSheet.qml
        MobileShell.qml
        NoteInputKeypad.qml
        SaveToLibraryDialog.qml
        TextEditBar.qml
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
        TARGET muse_learn_qml
        TARGET muse_shortcuts_qml
        TARGET muse_toast_qml
        TARGET appshell_qml
        TARGET notationscene_qml
        TARGET palette_qml
        TARGET inspector_qml
        TARGET instrumentsscene_qml
        TARGET playback_qml
        TARGET project_qml
)

fixup_qml_module_dependencies(mobile_qml)
