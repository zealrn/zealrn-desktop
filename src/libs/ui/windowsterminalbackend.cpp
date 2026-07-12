// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

#include <QWidget>

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
    QWidget *widget() const override { return nullptr; }
    bool isRunning() const override { return false; }
    bool start(const QString &, const QString &) override { return false; }
    void stop() override { }
    void clear() override { }
    void copy() override { }
    void paste() override { }
    void applyAppearance(bool) override { }
};

} // namespace

std::unique_ptr<TerminalBackend> createTerminalBackend(QWidget *parent)
{
    return std::make_unique<WindowsTerminalBackend>(parent);
}

} // namespace Zeal::WidgetUi
