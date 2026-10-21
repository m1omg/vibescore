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

#include <algorithm>

#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHash>
#include <QQuickWindow>
#include <QTimer>

#if defined(Q_OS_ANDROID)
#include "platform/android/androidshare.h"
#elif defined(Q_OS_IOS)
#include "platform/ios/iosplatform.h"
#elif defined(Q_OS_WASM)
#include <QTemporaryDir>
#include "platform/web/webplatform.h"
#endif

#include "translation.h"
#include "log.h"

using namespace mu::mobile;
using namespace muse;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
#define VS_SYSTEM_DOCUMENT_PICKER
#endif

// Scores are saved in the library and exports are given to the user (shared, downloaded) instead of file dialogs
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_WASM)
#define VS_LIBRARY_FILES
#endif

#ifdef Q_OS_WASM
//! The browser's file chooser: the chosen file is copied into the library (scores) or the cache folder
static void openFileFromDevice(const ScoresLibrary& library, const std::function<void(const io::path_t&)>& done)
{
    web::openFile(QString(), [&library, done](const QString& fileName, const QByteArray& content) {
        if (fileName.isEmpty()) {
            done(io::path_t());
            return;
        }

        QTemporaryDir tempDir;
        const QString tempPath = tempDir.filePath(QFileInfo(fileName).fileName());
        QFile tempFile(tempPath);
        if (!tempFile.open(QIODevice::WriteOnly) || tempFile.write(content) != content.size()) {
            LOGE() << "Can not write " << tempPath;
            done(io::path_t());
            return;
        }
        tempFile.close();

        done(library.importLocalFileByMoving(tempPath));
    });
}

//! The same, waiting for the user (a nested event loop: Asyncify)
static io::path_t openFileFromDeviceSync(const ScoresLibrary& library)
{
    QEventLoop loop;
    io::path_t result;
    openFileFromDevice(library, [&loop, &result](const io::path_t& path) {
        result = path;
        loop.quit();
    });
    loop.exec();
    return result;
}
#endif

static const Uri SAVE_TO_LIBRARY_URI("musescore://mobile/savetolibrary");

//! Unlike io::escapeFileName, keeps spaces: the name is shown in the Scores library
static QString validFileName(const QString& name)
{
    static const QString INVALID_CHARACTERS = QStringLiteral("/\\:*?\"<>|");

    QString result;
    for (const QChar& ch : name.trimmed()) {
        result += (INVALID_CHARACTERS.contains(ch) || ch.unicode() < 0x20) ? QChar('_') : ch;
    }

    return result;
}

static bool isScoreSaving(const std::vector<std::string>& filter)
{
    return std::any_of(filter.cbegin(), filter.cend(), [](const std::string& nameFilter) {
        return nameFilter.find("*.mscz") != std::string::npos;
    });
}

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

static const Uri BUSY_URI("musescore://mobile/busy");
//! The windows have drawn the busy dialog by then (unless they draw nothing: a page or an app in the background)
static constexpr int BUSY_DRAWN_TIMEOUT_MS = 1000;

//! Before a long task on the app's thread, during which nothing can be drawn: processes the events until each visible
//! window drew two frames, so that the windows show their latest changes (a dialog that opened, one that closed, ...);
//! user input waits until after the task
static void drawWindowsNow(int timeoutMs)
{
    // A browser shows a drawn frame at its next refresh, which can come after a task that starts at once (Firefox).
    // It asks for the next frame then: when a window drew its second frame, its first one is on the screen.
    static constexpr int FRAMES = 2;

    QHash<QQuickWindow*, int> framesToDraw;
    QEventLoop loop; // destroyed first, with the connections that use framesToDraw

    auto finished = [&loop, &framesToDraw]() {
        if (framesToDraw.isEmpty()) {
            // The long task starts in a later task, after the one that drew the last frame
            QTimer::singleShot(0, &loop, &QEventLoop::quit);
        }
    };

    for (QWindow* window : QGuiApplication::topLevelWindows()) {
        QQuickWindow* quickWindow = qobject_cast<QQuickWindow*>(window);
        if (!quickWindow || !quickWindow->isVisible()) {
            continue;
        }

        framesToDraw.insert(quickWindow, FRAMES);
        QObject::connect(quickWindow, &QQuickWindow::frameSwapped, &loop, [&framesToDraw, finished, quickWindow]() {
            auto it = framesToDraw.find(quickWindow);
            if (it == framesToDraw.end()) {
                return;
            }
            if (--it.value() > 0) {
                quickWindow->update();
                return;
            }
            framesToDraw.erase(it);
            finished();
        });
        // A window that goes away draws nothing more
        auto gone = [&framesToDraw, finished, quickWindow]() {
            if (framesToDraw.remove(quickWindow)) {
                finished();
            }
        };
        QObject::connect(quickWindow, &QObject::destroyed, &loop, gone);
        QObject::connect(quickWindow, &QWindow::visibleChanged, &loop, [gone](bool visible) {
            if (!visible) {
                gone();
            }
        });
        quickWindow->update();
    }

    if (framesToDraw.isEmpty()) {
        return;
    }

    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
}

void MobileInteractive::showProgress(const std::string& title, Progress progress)
{
    // What follows keeps the app's thread busy (e.g. opening a big score, see ProjectActionsController::doOpenProject),
    // and nothing can be drawn until it is done: the busy dialog is drawn before this returns, together with the other
    // changes of the windows (e.g. the dialog that asked about saving the previous score is gone). It has no progress
    // bar or Cancel button, which could neither move nor react meanwhile.
#ifdef Q_OS_WASM
    // The page shows the dialog instead, with a spinner that the browser keeps turning meanwhile (a page that shows
    // nothing moving for minutes, as a big score takes on a phone, looks frozen)
    web::showBusy(title);
#else
    UriQuery query(BUSY_URI);
    query.set("text", title);
    m_origin->open(query);
#endif

    progress.finished().onReceive(this, [this](const ProgressResult&) {
#ifdef Q_OS_WASM
        web::showBusy(std::string());
#else
        m_origin->close(BUSY_URI);
#endif
    });

    drawWindowsNow(BUSY_DRAWN_TIMEOUT_MS);
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
#elif defined(Q_OS_WASM)
    UNUSED(title);
    UNUSED(dir);
    UNUSED(filter);

    return async::make_promise<io::path_t>([this](auto resolve, auto reject) {
        openFileFromDevice(m_library, [resolve, reject](const io::path_t& path) {
            if (path.empty()) {
                Ret ret = make_ret(Ret::Code::Cancel);
                (void)reject(ret.code(), ret.text());
            } else {
                (void)resolve(path);
            }
        });

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
#elif defined(Q_OS_WASM)
    UNUSED(title);
    UNUSED(dir);
    UNUSED(filter);
    UNUSED(options);

    return openFileFromDeviceSync(m_library);
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
#elif defined(Q_OS_WASM)
    UNUSED(title);
    UNUSED(dir);
    UNUSED(filter);
    UNUSED(options);

    // One file at a time in the browser
    const io::path_t path = openFileFromDeviceSync(m_library);
    return path.empty() ? io::paths_t() : io::paths_t { path };
#else
    return m_origin->selectOpeningFilesSync(title, dir, filter, options);
#endif
}

io::path_t MobileInteractive::selectSavingFileSync(const std::string& title, const io::path_t& path, const std::vector<std::string>& filter,
                                                   bool confirmOverwrite)
{
#ifdef VS_LIBRARY_FILES
    UNUSED(title);

    if (isScoreSaving(filter)) {
        return selectScorePathInLibrary(path, confirmOverwrite);
    }

    return makeExportPath(path);
#else
    return m_origin->selectSavingFileSync(title, path, filter, confirmOverwrite);
#endif
}

io::path_t MobileInteractive::selectScorePathInLibrary(const io::path_t& suggestedPath, bool confirmOverwrite)
{
    const io::path_t libraryPath = m_library.libraryPath();
    QString name = io::completeBasename(suggestedPath).toQString();

    for (;;) {
        UriQuery query(SAVE_TO_LIBRARY_URI);
        query.set("name", name.toStdString());
        query.set("folderName", io::filename(libraryPath).toStdString());

        const RetVal<Val> rv = m_origin->openSync(query);
        if (!rv.ret) {
            return io::path_t();
        }

        name = validFileName(rv.val.toQString());
        if (name.isEmpty()) {
            continue;
        }

        const io::path_t target = libraryPath + "/" + name + ".mscz";

        if (!confirmOverwrite || !QFileInfo::exists(target.toQString())) {
            return target;
        }

        const Result result = m_origin->questionSync(muse::trc("mobile", "A score with this name already exists"),
                                                     muse::qtrc("mobile", "Do you want to replace “%1”?").arg(name).toStdString(),
                                                     m_origin->buttonDataList({ Button::Cancel, Button::Yes }), int(Button::Cancel));
        if (result.standardButton() == Button::Yes) {
            return target;
        }

        // Ask for another name
    }
}

io::path_t MobileInteractive::makeExportPath(const io::path_t& suggestedPath)
{
    // No dialog: exports go to the exports folder, then they are shared (see revealInFileBrowser)
    const io::path_t exportsPath = m_library.exportsPath();
    QDir().mkpath(exportsPath.toQString());

    m_lastExportTime = QDateTime::currentDateTime();

    return exportsPath + "/" + io::filename(suggestedPath);
}

QStringList MobileInteractive::lastExportedFiles(const io::path_t& exportPath) const
{
    // An export can write several files: one per page or part, named after the export path
    const QFileInfo exportInfo(exportPath.toQString());
    const QString pattern = exportInfo.completeBaseName() + "*." + exportInfo.suffix();

    QStringList files;
    const QFileInfoList candidates = QDir(exportInfo.absolutePath()).entryInfoList({ pattern }, QDir::Files, QDir::Name);
    for (const QFileInfo& candidate : candidates) {
        if (!m_lastExportTime.isValid() || candidate.lastModified() >= m_lastExportTime.addSecs(-1)) {
            files << candidate.absoluteFilePath();
        }
    }

    return files;
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
#ifdef VS_LIBRARY_FILES
    // There is no file browser to show files in: they are shared instead
    QStringList files;
    if (io::dirpath(filePath) == m_library.exportsPath()) {
        files = lastExportedFiles(filePath);
    } else if (QFileInfo(filePath.toQString()).isFile()) {
        files << filePath.toQString();
    }

    if (files.isEmpty()) {
        LOGE() << "Nothing to share for " << filePath;
        return make_ret(Ret::Code::UnknownError);
    }

#if defined(Q_OS_ANDROID)
    android::shareFiles(files, muse::qtrc("mobile", "Share"));
#elif defined(Q_OS_IOS)
    ios::shareFiles(files);
#else
    web::downloadFiles(files);
#endif
    return make_ok();
#else
    return m_origin->revealInFileBrowser(filePath);
#endif
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
