// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TERMINALBACKEND_H
#define ZEAL_WIDGETUI_TERMINALBACKEND_H

#include <QByteArray>
#include <QObject>
#include <QSize>
#include <QStringList>

#include <memory>

namespace Zeal::WidgetUi {

struct TerminalProfile
{
    QString id;
    QString label;
    QString program;
    QStringList arguments;
};

class TerminalBackend : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TerminalBackend)

public:
    explicit TerminalBackend(QObject *parent = nullptr);
    ~TerminalBackend() override = default;

    virtual bool isAvailable() const = 0;
    virtual QString unavailableReason() const = 0;
    virtual bool isRunning() const = 0;
    virtual bool start(const TerminalProfile &profile, const QString &workingDirectory, QSize initialSize) = 0;
    virtual void write(const QByteArray &data) = 0;
    virtual void resize(QSize size) = 0;
    virtual void interrupt() = 0;
    virtual void terminate() = 0;

signals:
    void outputReceived(const QByteArray &data);
    void started(const TerminalProfile &profile);
    void exited(int exitCode, bool exitCodeAvailable);
    void errorOccurred(const QString &message);
};

std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent = nullptr);

} // namespace Zeal::WidgetUi

Q_DECLARE_METATYPE(Zeal::WidgetUi::TerminalProfile)

#endif // ZEAL_WIDGETUI_TERMINALBACKEND_H
