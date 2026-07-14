// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

namespace Zeal::WidgetUi {

namespace {

class UnavailableTerminalBackend final : public TerminalBackend
{
public:
    explicit UnavailableTerminalBackend(QObject *parent = nullptr)
        : TerminalBackend(parent)
    {
    }

    bool isAvailable() const override { return false; }
    QString unavailableReason() const override
    {
#ifdef ZEALRN_ENABLE_TERMINAL
        return tr("An embedded terminal backend is not available in this build. Use Open External Terminal.");
#else
        return tr("Embedded terminal support was disabled when this application was built.");
#endif
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
    return std::make_unique<UnavailableTerminalBackend>(parent);
}

} // namespace Zeal::WidgetUi
