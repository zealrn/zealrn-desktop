// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TERMINALBACKEND_H
#define ZEAL_WIDGETUI_TERMINALBACKEND_H

#include <QObject>

#include <memory>

class QWidget;

namespace Zeal::WidgetUi {

class TerminalBackend : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalBackend)

public:
    explicit TerminalBackend(QObject *parent = nullptr);
    ~TerminalBackend() override = default;

    virtual bool isAvailable() const = 0;
    virtual QString unavailableReason() const = 0;
    virtual QWidget *widget() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool start(const QString &shell, const QString &workingDirectory) = 0;
    virtual void stop() = 0;
    virtual void clear() = 0;
    virtual void copy() = 0;
    virtual void paste() = 0;
    virtual void applyAppearance(bool dark) = 0;

signals:
    void started();
    void exited(int exitCode, bool exitCodeAvailable);
    void errorOccurred(const QString &message);
};

std::unique_ptr<TerminalBackend> createTerminalBackend(QWidget *parent = nullptr);

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_TERMINALBACKEND_H
