// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QVBoxLayout>
#include <QWidget>
#include <qtermwidget.h>

namespace Zeal::WidgetUi {

namespace {

class LinuxTerminalBackend final : public TerminalBackend
{
public:
    explicit LinuxTerminalBackend(QWidget *parent)
        : TerminalBackend(parent)
        , m_container(new QWidget(parent))
        , m_layout(new QVBoxLayout(m_container))
    {
        m_layout->setContentsMargins(0, 0, 0, 0);
    }

    ~LinuxTerminalBackend() override
    {
        stop();
        delete m_container;
    }

    bool isAvailable() const override { return true; }
    QString unavailableReason() const override { return {}; }
    QWidget *widget() const override { return m_container; }
    bool isRunning() const override { return m_running; }

    bool start(const QString &shell, const QString &workingDirectory) override
    {
        if (!QFileInfo(shell).isExecutable() || !QFileInfo(workingDirectory).isDir()) {
            emit errorOccurred(tr("The selected shell or working directory is no longer available."));
            return false;
        }

        stop();
        m_terminal = new QTermWidget(0, m_container);
        m_terminal->setScrollBarPosition(QTermWidget::ScrollBarRight);
        m_terminal->setHistorySize(5000);
        m_terminal->setShellProgram(shell);
        m_terminal->setWorkingDirectory(workingDirectory);
        m_terminal->setEnvironment(QProcessEnvironment::systemEnvironment().toStringList());
        m_layout->addWidget(m_terminal);
        applyAppearance(m_dark);

        QObject::connect(m_terminal, &QTermWidget::finished, this, [this]() {
            m_running = false;
            emit exited(0, false);
        });
        m_terminal->startShellProgram();
        m_running = true;
        emit started();
        return true;
    }

    void stop() override
    {
        if (!m_terminal) {
            return;
        }
        QObject::disconnect(m_terminal, nullptr, this, nullptr);
        delete m_terminal;
        m_terminal = nullptr;
        m_running = false;
    }

    void clear() override
    {
        if (m_terminal) {
            m_terminal->clear();
        }
    }

    void copy() override
    {
        if (m_terminal) {
            m_terminal->copyClipboard();
        }
    }

    void paste() override
    {
        if (m_terminal) {
            m_terminal->pasteClipboard();
        }
    }

    void applyAppearance(bool dark) override
    {
        m_dark = dark;
        if (!m_terminal) {
            return;
        }

        const QString preferred = dark ? QStringLiteral("DarkPastels") : QStringLiteral("BlackOnLightYellow");
        if (QTermWidget::availableColorSchemes().contains(preferred)) {
            m_terminal->setColorScheme(preferred);
        }
    }

private:
    QWidget *m_container = nullptr;
    QVBoxLayout *m_layout = nullptr;
    QTermWidget *m_terminal = nullptr;
    bool m_running = false;
    bool m_dark = false;
};

} // namespace

std::unique_ptr<TerminalBackend> createTerminalBackend(QWidget *parent)
{
    return std::make_unique<LinuxTerminalBackend>(parent);
}

} // namespace Zeal::WidgetUi
