# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2025 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

muse_create_qml_module(muse_graphicaleffects_qml ALIAS muse::graphicaleffects_qml FOR muse_uicomponents)

set(EFFECT_FILES
    EffectDropShadow.qml
    EffectRectangularGlow.qml
    EffectFastBlur.qml
    EffectColorOverlay.qml
)

if (MUSE_CONFIGURATION_IS_MOBILE)
    # Mobile platforms and the web: the effects of QtQuick.Effects, whose shaders are compiled when Qt is built.
    # Qt5Compat's effects compile theirs when the app runs, which fails on OpenGL ES 3 and WebGL 2
    # ("Failed to link shader program": the effect is not drawn), and needs Qt Shader Tools in the app.
    list(TRANSFORM EFFECT_FILES PREPEND "mobile/")
    foreach(effect_file ${EFFECT_FILES})
        get_filename_component(effect_name ${effect_file} NAME)
        set_source_files_properties(${effect_file} PROPERTIES QT_RESOURCE_ALIAS ${effect_name})
    endforeach()
endif()

qt_add_qml_module(muse_graphicaleffects_qml
    URI Muse.GraphicalEffects
    VERSION 1.0
    QML_FILES
        ${EFFECT_FILES}
        RoundedCornersEffect.qml
)

fixup_qml_module_dependencies(muse_graphicaleffects_qml)

qt_add_shaders(muse_graphicaleffects_qml muse_graphicaleffects_qml_shaders
    PREFIX
        "/qt/qml/Muse/GraphicalEffects"
    # The default versions, and GLSL ES 3.00 for OpenGL ES 3 and WebGL 2. The effect has its own vertex shader,
    # so that both stages get the same version (shaders of different versions do not link: the effect is not drawn)
    GLSL
        "100es,120,150,300es"
    FILES
        "shaders/roundcorners.vert"
        "shaders/roundcorners.frag"
)
