// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "developerterminalpanel.h"

#include "terminalbackend.h"
#include "terminalsupport.h"
#include "terminalview.h"

#include <core/settings.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

DeveloperTerminalPanel::DeveloperTerminalPanel(Core::Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel(tr("Shell"), this));
    m_shell = new QComboBox(this);
    m_shell->setObjectName(QStringLiteral("terminalShell"));
    const QList<TerminalProfile> profiles = TerminalSupport::availableTerminalProfiles();
    for (const TerminalProfile &profile : profiles) {
        m_shell->addItem(profile.label, profile.id);
    }
    const TerminalProfile selectedProfile = TerminalSupport::validatedTerminalProfile(m_settings->terminalShell, profiles);
    m_shell->setCurrentIndex(m_shell->findData(selectedProfile.id));
    toolbar->addWidget(m_shell);

    m_newSessionButton = new QPushButton(tr("New Session"), this);
    toolbar->addWidget(m_newSessionButton);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setObjectName(QStringLiteral("terminalStop"));
    toolbar->addWidget(m_stopButton);
    auto *workingDirectoryButton = new QPushButton(tr("Working Directory"), this);
    toolbar->addWidget(workingDirectoryButton);
    m_clearButton = new QPushButton(tr("Clear"), this);
    m_copyButton = new QPushButton(tr("Copy"), this);
    m_pasteButton = new QPushButton(tr("Paste"), this);
    toolbar->addWidget(m_clearButton);
    m_searchButton = new QPushButton(tr("Search"), this);
    m_searchButton->setObjectName(QStringLiteral("terminalSearch"));
    toolbar->addWidget(m_searchButton);
    toolbar->addWidget(m_copyButton);
    toolbar->addWidget(m_pasteButton);
    auto *decreaseFontButton = new QPushButton(QStringLiteral("-"), this);
    decreaseFontButton->setToolTip(tr("Decrease terminal font size"));
    m_fontSizeLabel = new QLabel(this);
    m_fontSizeLabel->setObjectName(QStringLiteral("terminalFontSize"));
    auto *increaseFontButton = new QPushButton(QStringLiteral("+"), this);
    increaseFontButton->setToolTip(tr("Increase terminal font size"));
    toolbar->addWidget(decreaseFontButton);
    toolbar->addWidget(m_fontSizeLabel);
    toolbar->addWidget(increaseFontButton);
    auto *externalButton = new QPushButton(tr("Open External Terminal"), this);
    toolbar->addWidget(externalButton);
    toolbar->addStretch();
    auto *closeButton = new QPushButton(tr("Close"), this);
    toolbar->addWidget(closeButton);
    layout->addLayout(toolbar);

    const QString workspace = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/workspace");
    m_workingDirectory = TerminalSupport::validatedWorkingDirectory(m_settings->terminalWorkingDirectory,
                                                                    QDir::homePath(),
                                                                    workspace);
    if (m_workingDirectory.isEmpty() && QDir().mkpath(workspace)) {
        m_workingDirectory = workspace;
    }

    auto *directoryRow = new QHBoxLayout();
    directoryRow->addWidget(new QLabel(tr("Directory:"), this));
    m_workingDirectoryLabel = new QLabel(m_workingDirectory, this);
    m_workingDirectoryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_workingDirectoryLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    directoryRow->addWidget(m_workingDirectoryLabel, 1);
    auto *copyPathButton = new QPushButton(tr("Copy Path"), this);
    directoryRow->addWidget(copyPathButton);
    auto *openFolderButton = new QPushButton(tr("Open Folder"), this);
    directoryRow->addWidget(openFolderButton);
    layout->addLayout(directoryRow);

    auto *statusRow = new QHBoxLayout();
    statusRow->addWidget(new QLabel(tr("Status:"), this));
    m_statusLabel = new QLabel(tr("Not started"), this);
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    layout->addLayout(statusRow);

    auto *terminalContainer = new QWidget(this);
    m_terminalLayout = new QVBoxLayout(terminalContainer);
    m_terminalLayout->setContentsMargins(0, 0, 0, 0);
    m_backendMessage = new QLabel(tr("Start a session or open an external terminal."), terminalContainer);
    m_backendMessage->setAlignment(Qt::AlignCenter);
    m_backendMessage->setWordWrap(true);
    m_terminalLayout->addWidget(m_backendMessage);
    layout->addWidget(terminalContainer, 1);

    auto *warning = new QLabel(
        tr("Developer Terminal uses your normal unrestricted user shell. Web Playground uses an isolated browser preview."),
        this);
    warning->setWordWrap(true);
    layout->addWidget(warning);

    m_newSessionButton->setEnabled(!profiles.isEmpty() && !m_workingDirectory.isEmpty());
    m_stopButton->setEnabled(false);
    m_clearButton->setEnabled(false);
    m_searchButton->setEnabled(false);
    m_copyButton->setEnabled(false);
    m_pasteButton->setEnabled(false);

    connect(m_newSessionButton, &QPushButton::clicked, this, &DeveloperTerminalPanel::startSession);
    connect(m_stopButton, &QPushButton::clicked, this, [this]() { m_terminalView->terminate(); });
    connect(workingDirectoryButton, &QPushButton::clicked, this, &DeveloperTerminalPanel::chooseWorkingDirectory);
    connect(m_clearButton, &QPushButton::clicked, this, [this]() { m_terminalView->clear(); });
    connect(m_searchButton, &QPushButton::clicked, this, [this]() {
        bool accepted = false;
        const QString text = QInputDialog::getText(this, tr("Search Terminal"), tr("Find:"), QLineEdit::Normal, {}, &accepted);
        if (accepted && !text.isEmpty()) {
            m_terminalView->search(text);
        }
    });
    connect(m_copyButton, &QPushButton::clicked, this, [this]() { m_terminalView->copy(); });
    connect(m_pasteButton, &QPushButton::clicked, this, [this]() { m_terminalView->paste(); });
    connect(externalButton, &QPushButton::clicked, this, &DeveloperTerminalPanel::openExternalTerminal);
    connect(closeButton, &QPushButton::clicked, this, &DeveloperTerminalPanel::closeRequested);
    connect(copyPathButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_workingDirectory);
    });
    connect(openFolderButton, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_workingDirectory));
    });
    connect(decreaseFontButton, &QPushButton::clicked, this, [this]() {
        setTerminalFontSize(m_settings->terminalFontSize - 1);
        m_settings->save();
    });
    connect(increaseFontButton, &QPushButton::clicked, this, [this]() {
        setTerminalFontSize(m_settings->terminalFontSize + 1);
        m_settings->save();
    });
    connect(m_settings, &Core::Settings::updated, this, &DeveloperTerminalPanel::applyAppearance);
    setTerminalFontSize(m_settings->terminalFontSize);
}

DeveloperTerminalPanel::~DeveloperTerminalPanel() = default;

void DeveloperTerminalPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureBackend();
    if (m_terminalView) {
        m_terminalView->focusTerminal();
    }
}

bool DeveloperTerminalPanel::acknowledgeSafety()
{
    if (m_settings->terminalSafetyAcknowledged) {
        return true;
    }

    const auto result = QMessageBox::warning(
        this,
        tr("Developer Terminal Access"),
        tr("The Developer Terminal has the same access as your normal shell. Commands can modify or delete files."),
        QMessageBox::Ok | QMessageBox::Cancel,
        QMessageBox::Cancel);
    if (result != QMessageBox::Ok) {
        return false;
    }

    m_settings->terminalSafetyAcknowledged = true;
    m_settings->save();
    return true;
}

void DeveloperTerminalPanel::ensureBackend()
{
    if (m_backend) {
        return;
    }

    m_backend = createTerminalBackend(this);
    connect(m_backend.get(), &TerminalBackend::started, this, [this](const TerminalProfile &) {
        updateStatus(tr("Running"));
        m_stopButton->setEnabled(true);
        m_clearButton->setEnabled(true);
        m_searchButton->setEnabled(true);
        m_copyButton->setEnabled(true);
        m_pasteButton->setEnabled(true);
    });
    connect(m_backend.get(), &TerminalBackend::exited, this, [this](int exitCode, bool available) {
        updateStatus(available ? tr("Exited (%1)").arg(exitCode) : tr("Exited"));
        m_stopButton->setEnabled(false);
        m_clearButton->setEnabled(false);
        m_searchButton->setEnabled(false);
        m_copyButton->setEnabled(false);
        m_pasteButton->setEnabled(false);
    });
    connect(m_backend.get(), &TerminalBackend::errorOccurred, this, [this](const QString &message) {
        updateStatus(tr("Error"));
        m_backendMessage->setText(message);
    });

    if (m_backend->isAvailable()) {
        m_terminalView = new TerminalView(m_backend.get(), this);
        m_terminalView->setObjectName(QStringLiteral("terminalView"));
        m_backendMessage->hide();
        m_terminalLayout->addWidget(m_terminalView);
    } else {
        m_backendMessage->setText(m_backend->unavailableReason());
    }
    applyAppearance();
    if (m_terminalView) {
        m_terminalView->setFontSize(m_settings->terminalFontSize);
    }
}

void DeveloperTerminalPanel::startSession()
{
    ensureBackend();
    if (!m_backend->isAvailable()) {
        updateStatus(tr("Embedded terminal unavailable"));
        return;
    }
    if (!acknowledgeSafety()) {
        return;
    }
    if (m_backend->isRunning()
        && QMessageBox::question(this,
                                 tr("Replace Terminal Session"),
                                 tr("Replace the running terminal session?"),
                                 QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No)
            != QMessageBox::Yes) {
        return;
    }

    if (m_backend->isRunning()) {
        m_backend->terminate();
    }

    m_settings->terminalShell = m_shell->currentData().toString();
    m_settings->terminalWorkingDirectory = m_workingDirectory;
    m_settings->save();
    const TerminalProfile profile = TerminalSupport::validatedTerminalProfile(m_settings->terminalShell,
                                                                               TerminalSupport::availableTerminalProfiles());
    if (!m_terminalView->start(profile, m_workingDirectory)) {
        updateStatus(tr("Failed to start"));
    } else {
        m_terminalView->focusTerminal();
    }
}

void DeveloperTerminalPanel::chooseWorkingDirectory()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Choose Working Directory"), m_workingDirectory);
    if (directory.isEmpty()) {
        return;
    }
    m_workingDirectory = QDir(directory).absolutePath();
    m_workingDirectoryLabel->setText(m_workingDirectory);
    m_settings->terminalWorkingDirectory = m_workingDirectory;
    m_settings->save();
}

void DeveloperTerminalPanel::openExternalTerminal()
{
    if (!acknowledgeSafety()) {
        return;
    }

    const auto launch = TerminalSupport::detectedExternalTerminal(m_shell->currentData().toString(), m_workingDirectory);
    if (!launch.isValid() || !QProcess::startDetached(launch.program, launch.arguments, m_workingDirectory)) {
        QMessageBox::warning(this,
                             tr("External Terminal Unavailable"),
                             tr("No external terminal could be started. Desktop containers may not be able to open a host terminal."));
    }
}

void DeveloperTerminalPanel::applyAppearance()
{
    if (m_terminalView) {
        m_terminalView->setDark(m_settings->isDarkModeEnabled());
    }
}

void DeveloperTerminalPanel::setTerminalFontSize(int size)
{
    const int boundedSize = TerminalSupport::clampTerminalFontSize(size);
    m_settings->terminalFontSize = boundedSize;
    m_fontSizeLabel->setText(tr("%1 px").arg(boundedSize));
    if (m_terminalView) {
        m_terminalView->setFontSize(boundedSize);
    }
}

void DeveloperTerminalPanel::updateStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

} // namespace Zeal::WidgetUi
