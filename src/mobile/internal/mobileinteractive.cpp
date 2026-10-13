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
#include "mobileinteractive.h"

#include <QFileDialog>

#include "log.h"

using namespace mu::mobile;
using namespace muse;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#define VS_SYSTEM_DOCUMENT_PICKER
#endif

MobileInteractive::MobileInteractive(std::shared_ptr<IInteractive> origin)
    : m_origin(std::move(origin))
{
}

IInteractive::ButtonData MobileInteractive::buttonData(Button b) const
{
    return m_origin->buttonData(b);
}

async::Promise<IInteractive::Result> MobileInteractive::question(const std::string& contentTitle, const Text& text,
                                                                 const ButtonDatas& buttons, int defBtn, const Options& options,
                                                                 const std::string& dialogTitle)
{
    return m_origin->question(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> MobileInteractive::info(const std::string& contentTitle, const Text& text,
                                                             const ButtonDatas& buttons, int defBtn, const Options& options,
                                                             const std::string& dialogTitle)
{
    return m_origin->info(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> MobileInteractive::warning(const std::string& contentTitle, const Text& text,
                                                                const ButtonDatas& buttons, int defBtn, const Options& options,
                                                                const std::string& dialogTitle)
{
    return m_origin->warning(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

async::Promise<IInteractive::Result> MobileInteractive::error(const std::string& contentTitle, const Text& text,
                                                              const ButtonDatas& buttons, int defBtn, const Options& options,
                                                              const std::string& dialogTitle)
{
    return m_origin->error(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

void MobileInteractive::showProgress(const std::string& title, Progress progress)
{
    m_origin->showProgress(title, progress);
}

// Files
//! The system document picker shows the documents of all apps and cloud storage. Name filters are not
//! passed on: Android does not know the MIME types of most score formats, and would hide those files.

async::Promise<io::path_t> MobileInteractive::selectOpeningFile(const std::string& title, const io::path_t& dir,
                                                                const std::vector<std::string>& filter)
{
#ifdef VS_SYSTEM_DOCUMENT_PICKER
    UNUSED(dir);
    UNUSED(filter);

    return async::make_promise<io::path_t>([this, title](auto resolve, auto reject) {
        QFileDialog* dialog = new QFileDialog(nullptr, QString::fromStdString(title));
        dialog->setFileMode(QFileDialog::ExistingFile);

        QObject::connect(dialog, &QFileDialog::finished, [this, dialog, resolve, reject](int result) {
            dialog->deleteLater();

            const QList<QUrl> urls = dialog->selectedUrls();
            if (result != QDialog::Accepted || urls.isEmpty()) {
                Ret ret = make_ret(Ret::Code::Cancel);
                (void)reject(ret.code(), ret.text());
                return;
            }

            const io::path_t path = m_library.importFile(urls.first());
            if (path.empty()) {
                Ret ret = make_ret(Ret::Code::UnknownError);
                (void)reject(ret.code(), ret.text());
                return;
            }

            (void)resolve(path);
        });

        dialog->open();

        return async::Promise<io::path_t>::Result::unchecked();
    }, async::PromiseType::AsyncByBody);
#else
    return m_origin->selectOpeningFile(title, dir, filter);
#endif
}

io::path_t MobileInteractive::selectOpeningFileSync(const std::string& title, const io::path_t& dir, const std::vector<std::string>& filter,
                                                    const int options)
{
#ifdef VS_SYSTEM_DOCUMENT_PICKER
    UNUSED(dir);
    UNUSED(filter);
    UNUSED(options);

    const QUrl url = QFileDialog::getOpenFileUrl(nullptr, QString::fromStdString(title));
    return url.isEmpty() ? io::path_t() : m_library.importFile(url);
#else
    return m_origin->selectOpeningFileSync(title, dir, filter, options);
#endif
}

io::paths_t MobileInteractive::selectOpeningFilesSync(const std::string& title, const io::path_t& dir,
                                                      const std::vector<std::string>& filter, const int options)
{
#ifdef VS_SYSTEM_DOCUMENT_PICKER
    UNUSED(dir);
    UNUSED(filter);
    UNUSED(options);

    io::paths_t paths;
    for (const QUrl& url : QFileDialog::getOpenFileUrls(nullptr, QString::fromStdString(title))) {
        const io::path_t path = m_library.importFile(url);
        if (!path.empty()) {
            paths.push_back(path);
        }
    }

    return paths;
#else
    return m_origin->selectOpeningFilesSync(title, dir, filter, options);
#endif
}

io::path_t MobileInteractive::selectSavingFileSync(const std::string& title, const io::path_t& path, const std::vector<std::string>& filter,
                                                   bool confirmOverwrite)
{
    return m_origin->selectSavingFileSync(title, path, filter, confirmOverwrite);
}

io::path_t MobileInteractive::selectDirectory(const std::string& title, const io::path_t& dir)
{
    return m_origin->selectDirectory(title, dir);
}

io::paths_t MobileInteractive::selectMultipleDirectories(const std::string& title, const io::path_t& dir,
                                                         const io::paths_t& selectedDirectories)
{
    return m_origin->selectMultipleDirectories(title, dir, selectedDirectories);
}

async::Promise<Color> MobileInteractive::selectColor(const Color& color, const std::string& title, bool allowAlpha)
{
    return m_origin->selectColor(color, title, allowAlpha);
}

bool MobileInteractive::isSelectColorOpened() const
{
    return m_origin->isSelectColorOpened();
}

// Custom

async::Promise<Val> MobileInteractive::open(const UriQuery& uri)
{
    return m_origin->open(uri);
}

RetVal<bool> MobileInteractive::isOpened(const UriQuery& uri) const
{
    return m_origin->isOpened(uri);
}

RetVal<bool> MobileInteractive::isOpened(const Uri& uri) const
{
    return m_origin->isOpened(uri);
}

async::Channel<Uri> MobileInteractive::opened() const
{
    return m_origin->opened();
}

void MobileInteractive::raise(const UriQuery& uri)
{
    m_origin->raise(uri);
}

async::Promise<Ret> MobileInteractive::close(const UriQuery& uri)
{
    return m_origin->close(uri);
}

async::Promise<Ret> MobileInteractive::close(const Uri& uri)
{
    return m_origin->close(uri);
}

Ret MobileInteractive::closeSync(const UriQuery& uri)
{
    return m_origin->closeSync(uri);
}

Ret MobileInteractive::closeAllDialogsSync()
{
    return m_origin->closeAllDialogsSync();
}

ValCh<Uri> MobileInteractive::currentUri() const
{
    return m_origin->currentUri();
}

RetVal<bool> MobileInteractive::isCurrentUriDialog() const
{
    return m_origin->isCurrentUriDialog();
}

std::vector<Uri> MobileInteractive::stack() const
{
    return m_origin->stack();
}

Ret MobileInteractive::openUrl(const std::string& url) const
{
    return m_origin->openUrl(url);
}

Ret MobileInteractive::openUrl(const QUrl& url) const
{
    return m_origin->openUrl(url);
}

Ret MobileInteractive::isAppExists(const std::string& appIdentifier) const
{
    return m_origin->isAppExists(appIdentifier);
}

Ret MobileInteractive::canOpenApp(const UriQuery& uri) const
{
    return m_origin->canOpenApp(uri);
}

async::Promise<Ret> MobileInteractive::openApp(const UriQuery& uri) const
{
    return m_origin->openApp(uri);
}

Ret MobileInteractive::revealInFileBrowser(const io::path_t& filePath) const
{
    return m_origin->revealInFileBrowser(filePath);
}

IInteractive::Result MobileInteractive::questionSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                     int defBtn, const Options& options, const std::string& dialogTitle)
{
    return m_origin->questionSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::Result MobileInteractive::infoSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                 int defBtn, const Options& options, const std::string& dialogTitle)
{
    return m_origin->infoSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::Result MobileInteractive::warningSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                    int defBtn, const Options& options, const std::string& dialogTitle)
{
    return m_origin->warningSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

IInteractive::Result MobileInteractive::errorSync(const std::string& contentTitle, const Text& text, const ButtonDatas& buttons,
                                                  int defBtn, const Options& options, const std::string& dialogTitle)
{
    return m_origin->errorSync(contentTitle, text, buttons, defBtn, options, dialogTitle);
}

RetVal<Val> MobileInteractive::openSync(const UriQuery& uri)
{
    return m_origin->openSync(uri);
}
