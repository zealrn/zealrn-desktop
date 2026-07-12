// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundeditor.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>

namespace Zeal::WidgetUi {

WebPlaygroundEditorBridge::WebPlaygroundEditorBridge(QObject *parent)
    : QObject(parent)
{
}

void WebPlaygroundEditorBridge::editorReady()
{
    emit ready();
}

void WebPlaygroundEditorBridge::contentChanged()
{
    emit changed();
}

WebPlaygroundEditor::WebPlaygroundEditor(bool dark, QWidget *parent)
    : QWidget(parent)
    , m_view(new QWebEngineView(this))
    , m_profile(new QWebEngineProfile(this))
    , m_channel(new QWebChannel(this))
    , m_bridge(new WebPlaygroundEditorBridge(this))
{
    m_profile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::NoPersistentCookies);

    auto *page = new QWebEnginePage(m_profile, m_view);
    page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    page->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    m_channel->registerObject(QStringLiteral("playgroundBridge"), m_bridge);
    page->setWebChannel(m_channel);
    m_view->setPage(page);
    m_view->installEventFilter(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);

    connect(m_bridge, &WebPlaygroundEditorBridge::ready, this, [this, dark]() {
        setDark(dark);
        emit ready();
    });
    connect(m_bridge, &WebPlaygroundEditorBridge::changed, this, &WebPlaygroundEditor::contentChanged);
    m_view->load(QUrl(QStringLiteral("qrc:/playground/editor.html")));
}

bool WebPlaygroundEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view && event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->matches(QKeySequence::Find)) {
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void WebPlaygroundEditor::requestDocuments(std::function<void(WebPlayground::Documents)> callback) const
{
    m_view->page()->runJavaScript(QStringLiteral("window.zealrnEditor.documents()"),
                                  [callback = std::move(callback)](const QVariant &result) {
        const QVariantMap values = result.toMap();
        callback({values.value(QStringLiteral("html")).toString(),
                  values.value(QStringLiteral("css")).toString(),
                  values.value(QStringLiteral("javascript")).toString()});
    });
}

void WebPlaygroundEditor::replaceDocuments(const WebPlayground::Documents &documents)
{
    const QJsonObject values = {{QStringLiteral("html"), documents.html},
                                {QStringLiteral("css"), documents.css},
                                {QStringLiteral("javascript"), documents.javascript}};
    runJavaScript(QStringLiteral("window.zealrnEditor.replaceDocuments(%1)")
                      .arg(QString::fromUtf8(QJsonDocument(values).toJson(QJsonDocument::Compact))));
}

void WebPlaygroundEditor::setActiveEditor(int index)
{
    static const QStringList names = {QStringLiteral("html"), QStringLiteral("css"), QStringLiteral("javascript")};
    if (index >= 0 && index < names.size()) {
        runJavaScript(QStringLiteral("window.zealrnEditor.setActive('%1')").arg(names.at(index)));
    }
}

void WebPlaygroundEditor::setDark(bool dark)
{
    runJavaScript(QStringLiteral("window.zealrnEditor.setDark(%1)").arg(dark ? QStringLiteral("true")
                                                                            : QStringLiteral("false")));
}

void WebPlaygroundEditor::runJavaScript(const QString &script) const
{
    m_view->page()->runJavaScript(script);
}

} // namespace Zeal::WidgetUi
