// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundpreview.h"

#include <QFileInfo>
#include <QVBoxLayout>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineUrlRequestInfo>
#include <QWebEngineView>

#include <utility>

namespace Zeal::WidgetUi {

namespace {
const QUrl PreviewUrl(QStringLiteral("qrc:/playground/preview.html"));
}

WebPlaygroundRequestInterceptor::WebPlaygroundRequestInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
{
}

void WebPlaygroundRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (WebPlayground::isBlockedPreviewUrl(info.requestUrl())) {
        const QUrl url = info.requestUrl();
        info.block(true);
        emit requestBlocked(url);
    }
}

WebPlaygroundPage::WebPlaygroundPage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent)
{
    connect(this,
            &QWebEnginePage::featurePermissionRequested,
            this,
            [this](const QUrl &origin, QWebEnginePage::Feature feature) {
        setFeaturePermission(origin, feature, QWebEnginePage::PermissionDeniedByUser);
    });
}

bool WebPlaygroundPage::acceptNavigationRequest(const QUrl &url, NavigationType, bool)
{
    return url == PreviewUrl;
}

QWebEnginePage *WebPlaygroundPage::createWindow(WebWindowType)
{
    return nullptr;
}

void WebPlaygroundPage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                                 const QString &message,
                                                 int lineNumber,
                                                 const QString &sourceId)
{
    QString severity = QStringLiteral("Info");
    if (level == QWebEnginePage::JavaScriptConsoleMessageLevel::WarningMessageLevel) {
        severity = QStringLiteral("Warning");
    } else if (level == QWebEnginePage::JavaScriptConsoleMessageLevel::ErrorMessageLevel) {
        severity = QStringLiteral("Error");
    }
    emit consoleMessage(severity, message, lineNumber, sourceId);
}

WebPlaygroundPreview::WebPlaygroundPreview(QWidget *parent)
    : QWidget(parent)
    , m_view(new QWebEngineView(this))
    , m_profile(new QWebEngineProfile(this))
    , m_page(new WebPlaygroundPage(m_profile, m_view))
{
    m_profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);
    auto *interceptor = new WebPlaygroundRequestInterceptor(m_profile);
    m_profile->setUrlRequestInterceptor(interceptor);

    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    m_page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    m_page->settings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, false);
    m_view->setPage(m_page);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(m_page, &WebPlaygroundPage::consoleMessage, this, &WebPlaygroundPreview::consoleMessage);
    connect(interceptor,
            &WebPlaygroundRequestInterceptor::requestBlocked,
            this,
            &WebPlaygroundPreview::requestBlocked,
            Qt::QueuedConnection);
    connect(m_view, &QWebEngineView::loadFinished, this, [this](bool ok) {
        if (!ok) {
            emit loadFailed();
            return;
        }
        if (!m_pendingRenderScript.isEmpty()) {
            const QString script = std::exchange(m_pendingRenderScript, {});
            m_page->runJavaScript(script);
        }
    });
    loadShell();
}

void WebPlaygroundPreview::render(const WebPlayground::Documents &documents)
{
    m_pendingRenderScript = WebPlayground::previewRenderScript(documents);
    loadShell();
}

void WebPlaygroundPreview::clear()
{
    m_pendingRenderScript.clear();
    loadShell();
}

void WebPlaygroundPreview::stop()
{
    m_pendingRenderScript.clear();
    m_view->stop();
    loadShell();
}

void WebPlaygroundPreview::loadShell()
{
    m_view->load(PreviewUrl);
}

} // namespace Zeal::WidgetUi
