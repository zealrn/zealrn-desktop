// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDPREVIEW_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDPREVIEW_H

#include "webplaygroundproject.h"

#include <QWebEnginePage>
#include <QWebEngineUrlRequestInterceptor>
#include <QWidget>

class QWebEngineProfile;
class QWebEngineView;
class QWebEngineUrlRequestInfo;

namespace Zeal::WidgetUi {

class WebPlaygroundRequestInterceptor final : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundRequestInterceptor)
public:
    explicit WebPlaygroundRequestInterceptor(QObject *parent = nullptr);

    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

signals:
    void requestBlocked(const QUrl &url);
};

class WebPlaygroundPage final : public QWebEnginePage
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundPage)
public:
    explicit WebPlaygroundPage(QWebEngineProfile *profile, QObject *parent = nullptr);

signals:
    void consoleMessage(const QString &severity,
                        const QString &message,
                        int lineNumber,
                        const QString &sourceId);

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    QWebEnginePage *createWindow(WebWindowType type) override;
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                  const QString &message,
                                  int lineNumber,
                                  const QString &sourceId) override;
};

class WebPlaygroundPreview final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundPreview)
public:
    explicit WebPlaygroundPreview(QWidget *parent = nullptr);
    ~WebPlaygroundPreview() override = default;

    void render(const WebPlayground::Documents &documents);
    void clear();
    void stop();

signals:
    void consoleMessage(const QString &severity,
                        const QString &message,
                        int lineNumber,
                        const QString &sourceId);
    void requestBlocked(const QUrl &url);
    void loadFailed();

private:
    void loadShell();

    QWebEngineView *m_view = nullptr;
    QWebEngineProfile *m_profile = nullptr;
    WebPlaygroundPage *m_page = nullptr;
    QString m_pendingRenderScript;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDPREVIEW_H
