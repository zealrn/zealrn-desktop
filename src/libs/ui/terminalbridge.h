// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TERMINALBRIDGE_H
#define ZEAL_WIDGETUI_TERMINALBRIDGE_H

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QString>

namespace Zeal::WidgetUi {

class TerminalBridge final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalBridge)

public:
    static constexpr qsizetype MaxPendingBytes = 4 * 1024 * 1024;

    explicit TerminalBridge(QObject *parent = nullptr);

    void enqueueOutput(const QByteArray &data);

public slots:
    void frontendReady();
    void sendInput(const QString &encodedData);
    void resizeTerminal(int columns, int rows);
    void startSession(const QString &shellId, const QString &workingDirectory);
    void interruptSession();
    void terminateSession();
    void copySelection(const QString &text);
    void requestPaste();

signals:
    void outputReceived(const QString &encodedData);
    void sessionStarted(const QString &profile);
    void sessionExited(int exitCode, bool exitCodeAvailable);
    void themeChanged(const QString &theme);
    void fontSizeChanged(int size);
    void clearRequested();
    void focusRequested();
    void copyRequested();
    void pasteReceived(const QString &encodedData);
    void searchRequested(const QString &text, bool backward);
    void errorOccurred(const QString &message);

    void inputReceived(const QByteArray &data);
    void terminalResizeRequested(const QSize &size);
    void sessionStartRequested(const QString &shellId, const QString &workingDirectory);
    void interruptRequested();
    void terminateRequested();
    void selectionCopyRequested(const QString &text);
    void pasteRequested();

private:
    void scheduleFlush();
    void flushOutput();

    QByteArray m_pendingOutput;
    bool m_frontendReady = false;
    bool m_flushScheduled = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_TERMINALBRIDGE_H
