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

#include <QString>
#include <QStringList>

namespace mu::mobile::ios {
//! Shows the share sheet (UIActivityViewController) with the files: AirDrop, mail, messages, "Save to Files", ...
void shareFiles(const QStringList& paths);

//! Shows the print dialog (UIPrintInteractionController) for a PDF file
void printPdf(const QString& pdfPath, const QString& jobName);

//! Shows Apple's panel of nearby Bluetooth MIDI devices (CABTMIDICentralViewController); connected devices are
//! CoreMIDI devices like the others
void showBluetoothMidiDevices();
}
