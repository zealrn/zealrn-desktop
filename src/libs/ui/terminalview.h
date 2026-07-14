// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TERMINALVIEW_H
#define ZEAL_WIDGETUI_TERMINALVIEW_H

#include <QUrl>
#include <QWidget>

class QWebChannel;
class QWebEngineProfile;
class QWebEngineView;

namespace Zeal::WidgetUi {

class TerminalBackend;
class TerminalBridge;
struct TerminalProfile;

class TerminalView final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalView)

public:
    explicit TerminalView(TerminalBackend *backend, QWidget *parent = nullptr);
    ~TerminalView() override = default;

    static bool isAllowedUrl(const QUrl &url);

    bool start(const TerminalProfile &profile, const QString &workingDirectory);
    void terminate();
    void interrupt();
    void clear();
    void copy();
    void paste();
    void search(const QString &text, bool backward = false);
    void setDark(bool dark);
    void setFontSize(int size);
    void focusTerminal();
    bool isReady() const;

signals:
    void ready();

private:
    void sendClipboard();

    TerminalBackend *m_backend = nullptr;
    QWebEngineView *m_view = nullptr;
    QWebEngineProfile *m_profile = nullptr;
    QWebChannel *m_channel = nullptr;
    TerminalBridge *m_bridge = nullptr;
    QSize m_terminalSize = QSize(80, 24);
    int m_fontSize = 14;
    bool m_dark = false;
    bool m_ready = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_TERMINALVIEW_H
