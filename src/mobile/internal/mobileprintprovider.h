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
#pragma once

#include "modularity/ioc.h"
#include "print/iprintprovider.h"
#include "project/inotationwritersregister.h"

namespace mu::mobile {
//! Printing on mobile platforms: the score is written to a PDF file, which the system prints
//! (Android: the print dialog, with the printers of the device and "Save as PDF")
class MobilePrintProvider : public print::IPrintProvider, public muse::Contextable
{
    muse::ContextInject<project::INotationWritersRegister> writers = { this };

public:
    explicit MobilePrintProvider(const muse::modularity::ContextPtr& iocCtx);

    muse::Ret printNotation(notation::INotationPtr notation) override;
};
}
