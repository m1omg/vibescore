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
#include <gmock/gmock.h>

#include <QGuiApplication>
#include <QWindow>

#include "ui/view/interactiveprovider.h"
#include "ui/iinteractiveuriregister.h"

#include "mocks/mainwindowmock.h"

using ::testing::NiceMock;
using ::testing::Return;

using namespace muse;
using namespace muse::ui;

static const std::string TEST_MODULE = "interactiveprovider_tests";
static const Uri PAGE_URI("muse://test/page");
static const Uri DIALOG_URI("muse://test/dialog");

class TestUriRegister : public IInteractiveUriRegister
{
public:
    void unregisterUri(const Uri& uri) override
    {
        m_metas.erase(uri.toString());
    }

    ContainerMeta meta(const Uri& uri) const override
    {
        auto it = m_metas.find(uri.toString());
        return it != m_metas.end() ? it->second : ContainerMeta();
    }

protected:
    void registerUri(const Uri& uri, const ContainerMeta& meta) override
    {
        m_metas[uri.toString()] = meta;
    }

private:
    std::map<std::string, ContainerMeta> m_metas;
};

//! Dialogs shown inside the main window (single window platforms, see WindowView::isInWindow)
//! are identified by their own object instead of a window. They must still count as the current dialog,
//! otherwise the shortcuts of the page below (e.g. Delete on the score) would act while the dialog is open.
class InteractiveProviderTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_uriRegister = std::make_shared<TestUriRegister>();
        m_uriRegister->registerPageUri(PAGE_URI);
        m_uriRegister->registerQmlUri(DIALOG_URI, "Test.Dialogs", "TestDialog");
        modularity::globalIoc()->registerExport<IInteractiveUriRegister>(TEST_MODULE, m_uriRegister);

        m_mainWindow = std::make_shared<NiceMock<MainWindowMock> >();
        ON_CALL(*m_mainWindow, qWindow()).WillByDefault(Return(&m_window));
        modularity::globalIoc()->registerExport<IMainWindow>(TEST_MODULE, m_mainWindow);

        m_provider = new InteractiveProvider(modularity::globalCtx());

        // What InteractiveProvider.qml does: create the object, then report it
        QObject::connect(m_provider, &InteractiveProvider::fireOpen, m_provider, [this](QmlLaunchData* data) {
            const QString uri = data->value("uri").toString();
            const bool isPage = uri == QString::fromStdString(PAGE_URI.toString());
            const QString objectId = isPage ? "page" : "dialog";

            data->setValue("ret", QVariantMap { { "errcode", 0 } });
            data->setValue("objectId", objectId);

            m_provider->onOpen(QVariant::fromValue(isPage ? ContainerType::PrimaryPage : ContainerType::QmlDialog),
                               objectId, isPage ? nullptr : m_dialogObject);
        }, Qt::DirectConnection);
    }

    void TearDown() override
    {
        delete m_provider;
        modularity::globalIoc()->unregisterIfRegistered<IMainWindow>(TEST_MODULE, m_mainWindow);
        modularity::globalIoc()->unregisterIfRegistered<IInteractiveUriRegister>(TEST_MODULE, m_uriRegister);
    }

    QWindow m_window;
    QObject* m_dialogObject = nullptr;

    std::shared_ptr<TestUriRegister> m_uriRegister;
    std::shared_ptr<MainWindowMock> m_mainWindow;
    InteractiveProvider* m_provider = nullptr;
};

TEST_F(InteractiveProviderTests, Dialog_Inside_Main_Window_Is_Current_Dialog)
{
    //! [GIVEN] The main window shows a page
    m_provider->openAsync(UriQuery(PAGE_URI));
    QCoreApplication::processEvents();

    EXPECT_EQ(m_provider->currentUri().val, PAGE_URI);
    EXPECT_FALSE(m_provider->isCurrentUriDialog().val);

    //! [WHEN] A dialog opens inside the main window
    QObject inWindowDialog;
    inWindowDialog.setProperty("window", QVariant::fromValue(&m_window));
    m_dialogObject = &inWindowDialog;

    m_provider->openAsync(UriQuery(DIALOG_URI));
    QCoreApplication::processEvents();

    //! [THEN] It is the current dialog, hosted by the main window
    EXPECT_EQ(m_provider->currentUri().val, DIALOG_URI);
    EXPECT_TRUE(m_provider->isCurrentUriDialog().val);
    EXPECT_EQ(m_provider->topWindow(), &m_window);

    //! [WHEN] The main window gets the focus (e.g. after a tap on the dialog, which is part of it)
    emit qGuiApp->focusWindowChanged(&m_window);

    //! [THEN] The dialog stays the current one
    EXPECT_EQ(m_provider->currentUri().val, DIALOG_URI);
    EXPECT_TRUE(m_provider->isCurrentUriDialog().val);

    //! [WHEN] The dialog closes
    m_provider->onClose("dialog", QVariantMap { { "errcode", 0 } });

    //! [THEN] The page is current again
    EXPECT_EQ(m_provider->currentUri().val, PAGE_URI);
    EXPECT_FALSE(m_provider->isCurrentUriDialog().val);
}
