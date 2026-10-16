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
#include "ui/iinteractiveuriregister.h"

#include "internal/backnavigationcontroller.h"
#include "internal/bluetoothmidicontroller.h"
#include "internal/incomingfilescontroller.h"
#include "internal/mobileinteractive.h"
#include "internal/mobilelifecyclecontroller.h"
#include "internal/mobileprintprovider.h"
#include "internal/mobilewindowsprovider.h"

#ifdef Q_OS_WASM
#include "audio/main/isoundfontcontroller.h"
#include "internal/platform/web/webplatform.h"
#endif

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
    m_backNavigationController = std::make_shared<BackNavigationController>(iocContext());
    m_bluetoothMidiController = std::make_shared<BluetoothMidiController>(iocContext());
    m_lifecycleController = std::make_shared<MobileLifecycleController>(iocContext());

    // On top of the default IInteractive (registered by the global module): system document picker, Scores library
    std::shared_ptr<IInteractive> originInteractive = ioc()->resolve<IInteractive>(moduleName());
    ioc()->unregister<IInteractive>(moduleName());
    ioc()->registerExport<IInteractive>(moduleName(), std::make_shared<MobileInteractive>(originInteractive));

    // The print module (Qt Print Support) is not available on mobile platforms
    ioc()->registerExport<mu::print::IPrintProvider>(moduleName(), std::make_shared<MobilePrintProvider>(iocContext()));

    // Replaces the stub of the multiwindows module, which is registered before this module
    ioc()->unregister<muse::mi::IMultiWindowsProvider>(moduleName());
    ioc()->registerExport<muse::mi::IMultiWindowsProvider>(moduleName(), std::make_shared<MobileWindowsProvider>(iocContext()));
}

void MobileModule::resolveImports()
{
    auto ir = ioc()->resolve<muse::ui::IInteractiveUriRegister>(moduleName());
    if (ir) {
        ir->registerQmlUri(Uri("musescore://mobile/savetolibrary"), "MuseScore.Mobile", "SaveToLibraryDialog");
        ir->registerQmlUri(Uri("musescore://mobile/busy"), "MuseScore.Mobile", "BusyDialog");
    }
}

void MobileModule::onInit(const IApplication::RunMode& mode)
{
    if (mode != IApplication::RunMode::GuiApp) {
        return;
    }

    m_incomingFilesController->init();
    m_backNavigationController->init();
    m_bluetoothMidiController->init();
    m_lifecycleController->init();

#ifdef Q_OS_WASM
    web::fixNestedEventLoops();

    // The page downloads the sound font after the app started: then it is loaded like the others
    web::setSoundFontsChangedHandler([this]() {
        if (auto soundFonts = ioc()->resolve<muse::audio::ISoundFontController>(moduleName())) {
            soundFonts->loadSoundFonts();
        }
    });
#endif
}

void MobileModule::onDeinit()
{
    m_lifecycleController.reset();
    m_backNavigationController.reset();
    m_bluetoothMidiController.reset();
    m_incomingFilesController.reset();
}
