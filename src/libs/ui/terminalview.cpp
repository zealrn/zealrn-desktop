// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalview.h"

#include "terminalbackend.h"
#include "terminalbridge.h"

#include <QApplication>
#include <QClipboard>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QWebEnginePermission>
#endif
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineView>

namespace Zeal::WidgetUi {

namespace {
const QUrl TerminalUrl(QStringLiteral("qrc:/terminal/index.html"));

class TerminalRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
public:
    using QWebEngineUrlRequestInterceptor::QWebEngineUrlRequestInterceptor;

    void interceptRequest(QWebEngineUrlRequestInfo &info) override
    {
        if (!TerminalView::isAllowedUrl(info.requestUrl())) {
            info.block(true);
        }
    }
};

class TerminalPage final : public QWebEnginePage
{
public:
    TerminalPage(QWebEngineProfile *profile, QObject *parent)
        : QWebEnginePage(profile, parent)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        connect(this, &QWebEnginePage::permissionRequested, this, [](QWebEnginePermission permission) {
            permission.deny();
        });
#else
        connect(this,
                &QWebEnginePage::featurePermissionRequested,
                this,
                [this](const QUrl &origin, QWebEnginePage::Feature feature) {
            setFeaturePermission(origin, feature, QWebEnginePage::PermissionDeniedByUser);
        });
#endif
    }

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType, bool) override { return url == TerminalUrl; }
    QWebEnginePage *createWindow(WebWindowType) override { return nullptr; }
};
} // namespace

TerminalView::TerminalView(TerminalBackend *backend, QWidget *parent)
    : QWidget(parent)
    , m_backend(backend)
    , m_view(new QWebEngineView(this))
    , m_profile(new QWebEngineProfile(this))
    , m_channel(new QWebChannel(this))
    , m_bridge(new TerminalBridge(this))
{
    m_profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    m_profile->setUrlRequestInterceptor(new TerminalRequestInterceptor(m_profile));

    auto *page = new TerminalPage(m_profile, m_view);
    page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    page->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
    m_channel->registerObject(QStringLiteral("terminalBridge"), m_bridge);
    page->setWebChannel(m_channel);
    m_view->setPage(page);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(m_backend, &TerminalBackend::outputReceived, m_bridge, &TerminalBridge::enqueueOutput);
    connect(m_backend, &TerminalBackend::started, m_bridge, [this](const TerminalProfile &profile) {
        emit m_bridge->sessionStarted(profile.label);
    });
    connect(m_backend, &TerminalBackend::exited, m_bridge, &TerminalBridge::sessionExited);
    connect(m_backend, &TerminalBackend::errorOccurred, m_bridge, &TerminalBridge::errorOccurred);
    connect(m_bridge, &TerminalBridge::inputReceived, m_backend, &TerminalBackend::write);
    connect(m_bridge, &TerminalBridge::terminalResizeRequested, this, [this](QSize size) {
        m_terminalSize = size;
        m_backend->resize(size);
    });
    connect(m_bridge, &TerminalBridge::interruptRequested, m_backend, &TerminalBackend::interrupt);
    connect(m_bridge, &TerminalBridge::terminateRequested, m_backend, &TerminalBackend::terminate);
    connect(m_bridge, &TerminalBridge::selectionCopyRequested, this, [](const QString &text) {
        QApplication::clipboard()->setText(text);
    });
    connect(m_bridge, &TerminalBridge::pasteRequested, this, &TerminalView::sendClipboard);

    m_view->load(TerminalUrl);
}

bool TerminalView::isAllowedUrl(const QUrl &url)
{
    if (url.scheme() != QStringLiteral("qrc")) {
        return false;
    }
    return url.path().startsWith(QStringLiteral("/terminal/"))
        || url.path() == QStringLiteral("/qtwebchannel/qwebchannel.js");
}

bool TerminalView::start(const TerminalProfile &profile, const QString &workingDirectory)
{
    return m_backend->start(profile, workingDirectory, m_terminalSize);
}

void TerminalView::terminate()
{
    m_backend->terminate();
}

void TerminalView::interrupt()
{
    m_backend->interrupt();
}

void TerminalView::clear()
{
    emit m_bridge->clearRequested();
}

void TerminalView::copy()
{
    emit m_bridge->copyRequested();
}

void TerminalView::paste()
{
    sendClipboard();
}

void TerminalView::search(const QString &text, bool backward)
{
    emit m_bridge->searchRequested(text, backward);
}

void TerminalView::setDark(bool dark)
{
    emit m_bridge->themeChanged(dark ? QStringLiteral("dark") : QStringLiteral("light"));
}

void TerminalView::setFontSize(int size)
{
    emit m_bridge->fontSizeChanged(size);
}

void TerminalView::focusTerminal()
{
    emit m_bridge->focusRequested();
}

void TerminalView::sendClipboard()
{
    const QByteArray data = QApplication::clipboard()->text().toUtf8();
    emit m_bridge->pasteReceived(QString::fromLatin1(data.toBase64()));
}

} // namespace Zeal::WidgetUi
