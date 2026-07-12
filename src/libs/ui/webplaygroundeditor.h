// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDEDITOR_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDEDITOR_H

#include "webplaygroundproject.h"

#include <QObject>
#include <QWidget>

#include <functional>

class QWebChannel;
class QWebEngineProfile;
class QWebEngineView;

namespace Zeal::WidgetUi {

class WebPlaygroundEditorBridge final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundEditorBridge)
public:
    explicit WebPlaygroundEditorBridge(QObject *parent = nullptr);

public slots:
    void editorReady();
    void contentChanged();

signals:
    void ready();
    void changed();
};

class WebPlaygroundEditor final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundEditor)
public:
    explicit WebPlaygroundEditor(bool dark, QWidget *parent = nullptr);
    ~WebPlaygroundEditor() override = default;

    void requestDocuments(std::function<void(WebPlayground::Documents)> callback) const;
    void replaceDocuments(const WebPlayground::Documents &documents);
    void setActiveEditor(int index);
    void setDark(bool dark);

signals:
    void ready();
    void contentChanged();

private:
    void runJavaScript(const QString &script) const;

    QWebEngineView *m_view = nullptr;
    QWebEngineProfile *m_profile = nullptr;
    QWebChannel *m_channel = nullptr;
    WebPlaygroundEditorBridge *m_bridge = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDEDITOR_H
