// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

namespace Zeal::WidgetUi {

namespace {

class WindowsTerminalBackend final : public TerminalBackend
{
public:
    explicit WindowsTerminalBackend(QObject *parent = nullptr)
        : TerminalBackend(parent)
    {
    }

    bool isAvailable() const override { return false; }
    QString unavailableReason() const override
    {
        return tr("Embedded ConPTY support requires a tested terminal renderer and is not available yet. Use Open External Terminal.");
    }
    bool isRunning() const override { return false; }
    bool start(const TerminalProfile &, const QString &, QSize) override { return false; }
    void write(const QByteArray &) override { }
    void resize(QSize) override { }
    void interrupt() override { }
    void terminate() override { }
};

} // namespace

std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent)
{
    return std::make_unique<WindowsTerminalBackend>(parent);
}

} // namespace Zeal::WidgetUi
