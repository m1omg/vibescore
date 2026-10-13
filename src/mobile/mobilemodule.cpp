/*
 * SPDX-License-Identifier: GPL-3.0-only
 * VibeScore - MuseScore Studio for mobile devices
 *
 * Copyright (C) 2026 VibeScore contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "mobilemodule.h"

#include "modularity/ioc.h"

#include "internal/incomingfilescontroller.h"

using namespace mu::mobile;
using namespace muse;
using namespace muse::modularity;

std::string MobileModule::moduleName() const
{
    return "mobile";
}

void MobileModule::registerExports()
{
    m_incomingFilesController = std::make_shared<IncomingFilesController>(iocContext());
}

void MobileModule::onInit(const IApplication::RunMode& mode)
{
    if (mode != IApplication::RunMode::GuiApp) {
        return;
    }

    m_incomingFilesController->init();
}

void MobileModule::onDeinit()
{
    m_incomingFilesController.reset();
}
