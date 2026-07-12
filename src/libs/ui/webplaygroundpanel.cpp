// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundpanel.h"

#include "webplaygroundeditor.h"
#include "webplaygroundpreview.h"
#include "webplaygroundproject.h"

#include <core/settings.h>

#include <QCheckBox>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QPointer>
#include <QHideEvent>
#include <QShowEvent>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QTime>
#include <QTimer>
#include <QTemporaryDir>
#include <QUrl>

namespace Zeal::WidgetUi {

WebPlaygroundPanel::WebPlaygroundPanel(Core::Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_autoRunTimer(new QTimer(this))
{
    m_autoRunTimer->setSingleShot(true);
    m_autoRunTimer->setInterval(600);
    m_consoleRateTimer.start();
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *editorControls = new QHBoxLayout();
    m_editorTabs = new QTabBar(this);
    m_editorTabs->addTab(tr("HTML"));
    m_editorTabs->addTab(tr("CSS"));
    m_editorTabs->addTab(tr("JavaScript"));
    editorControls->addWidget(m_editorTabs);
    editorControls->addStretch();

    m_runButton = new QPushButton(tr("Run"), this);
    m_runButton->setEnabled(false);
    editorControls->addWidget(m_runButton);

    m_autoRun = new QCheckBox(tr("Auto Run"), this);
    m_autoRun->setEnabled(false);
    editorControls->addWidget(m_autoRun);
    layout->addLayout(editorControls);

    auto *editorContainer = new QWidget(this);
    m_editorLayout = new QVBoxLayout(editorContainer);
    m_editorLayout->setContentsMargins(0, 0, 0, 0);
    m_editorPlaceholder = new QLabel(tr("Code editor loads when the playground opens."), editorContainer);
    m_editorPlaceholder->setAlignment(Qt::AlignCenter);
    m_editorPlaceholder->setFrameShape(QFrame::StyledPanel);
    m_editorLayout->addWidget(m_editorPlaceholder);
    layout->addWidget(editorContainer, 1);

    m_outputTabs = new QTabWidget(this);
    m_previewPlaceholder = new QLabel(tr("Run the code to update the preview."), m_outputTabs);
    m_previewPlaceholder->setAlignment(Qt::AlignCenter);
    m_outputTabs->addTab(m_previewPlaceholder, tr("Preview"));
    m_console = new QListWidget(m_outputTabs);
    m_console->setUniformItemSizes(true);
    m_outputTabs->addTab(m_console, tr("Console"));
    layout->addWidget(m_outputTabs, 1);

    auto *warning = new QLabel(tr("Code runs locally inside an isolated preview. Do not run untrusted code."), this);
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto *commands = new QHBoxLayout();
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setEnabled(false);
    commands->addWidget(m_stopButton);
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_resetButton->setEnabled(false);
    commands->insertWidget(1, m_resetButton);
    m_clearConsoleButton = new QPushButton(tr("Clear Console"), this);
    m_clearConsoleButton->setEnabled(false);
    commands->addWidget(m_clearConsoleButton);
    m_openBrowserButton = new QPushButton(tr("Open in Browser"), this);
    m_openBrowserButton->setEnabled(false);
    commands->addWidget(m_openBrowserButton);
    m_exportButton = new QPushButton(tr("Export Project"), this);
    m_exportButton->setEnabled(false);
    commands->addWidget(m_exportButton);
    commands->addStretch();

    auto *closeButton = new QPushButton(tr("Close Playground"), this);
    connect(closeButton, &QPushButton::clicked, this, &WebPlaygroundPanel::closeRequested);
    commands->addWidget(closeButton);
    layout->addLayout(commands);

    connect(m_settings, &Core::Settings::updated, this, &WebPlaygroundPanel::applyAppearance);
    connect(m_runButton, &QPushButton::clicked, this, &WebPlaygroundPanel::runPreview);
    connect(m_stopButton, &QPushButton::clicked, this, &WebPlaygroundPanel::stopPreview);
    connect(m_resetButton, &QPushButton::clicked, this, &WebPlaygroundPanel::resetDocuments);
    connect(m_clearConsoleButton, &QPushButton::clicked, this, &WebPlaygroundPanel::clearConsole);
    connect(m_openBrowserButton, &QPushButton::clicked, this, &WebPlaygroundPanel::openInBrowser);
    connect(m_exportButton, &QPushButton::clicked, this, &WebPlaygroundPanel::exportProject);
    connect(m_autoRunTimer, &QTimer::timeout, this, &WebPlaygroundPanel::runPreview);
    connect(m_autoRun, &QCheckBox::toggled, this, [this](bool enabled) {
        if (enabled && isVisible() && m_editor != nullptr) {
            m_autoRunTimer->start();
        } else {
            m_autoRunTimer->stop();
        }
    });
}

WebPlaygroundPanel::~WebPlaygroundPanel() = default;

void WebPlaygroundPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureInitialized();
}

void WebPlaygroundPanel::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_autoRunTimer->stop();
    ++m_runGeneration;
}

void WebPlaygroundPanel::ensureInitialized()
{
    if (m_editor != nullptr) {
        return;
    }

    m_editor = new WebPlaygroundEditor(m_settings->isDarkModeEnabled(), this);
    m_editorLayout->replaceWidget(m_editorPlaceholder, m_editor);
    m_editorPlaceholder->deleteLater();
    m_editorPlaceholder = nullptr;

    m_preview = new WebPlaygroundPreview(this);
    const int previewIndex = m_outputTabs->indexOf(m_previewPlaceholder);
    m_outputTabs->removeTab(previewIndex);
    m_previewPlaceholder->deleteLater();
    m_previewPlaceholder = nullptr;
    m_outputTabs->insertTab(previewIndex, m_preview, tr("Preview"));
    m_outputTabs->setCurrentIndex(previewIndex);

    connect(m_editorTabs, &QTabBar::currentChanged, m_editor, &WebPlaygroundEditor::setActiveEditor);
    connect(m_editor, &WebPlaygroundEditor::ready, this, [this]() {
        m_runButton->setEnabled(true);
        m_autoRun->setEnabled(true);
        m_resetButton->setEnabled(true);
        m_stopButton->setEnabled(true);
        m_clearConsoleButton->setEnabled(true);
        m_openBrowserButton->setEnabled(true);
        m_exportButton->setEnabled(true);
        m_editor->setActiveEditor(m_editorTabs->currentIndex());
    });
    connect(m_editor, &WebPlaygroundEditor::contentChanged, this, [this]() {
        if (m_autoRun->isChecked() && isVisible()) {
            m_autoRunTimer->start();
        }
    });
    connect(m_preview, &WebPlaygroundPreview::consoleMessage, this, &WebPlaygroundPanel::appendConsole);
    connect(m_preview, &WebPlaygroundPreview::requestBlocked, this, [this](const QUrl &url) {
        appendConsole(QStringLiteral("Warning"), tr("Blocked external request: %1").arg(url.toDisplayString()));
    });
    connect(m_preview, &WebPlaygroundPreview::loadFailed, this, [this]() {
        appendConsole(QStringLiteral("Error"), tr("Preview failed to load."));
    });
}

void WebPlaygroundPanel::applyAppearance()
{
    if (m_editor != nullptr) {
        m_editor->setDark(m_settings->isDarkModeEnabled());
    }
    updateConsoleColors();
}

void WebPlaygroundPanel::runPreview()
{
    if (m_editor == nullptr || m_preview == nullptr || !isVisible()) {
        return;
    }

    m_autoRunTimer->stop();
    const quint64 generation = ++m_runGeneration;
    const QPointer<WebPlaygroundPanel> guard(this);
    m_editor->requestDocuments([guard, generation](WebPlayground::Documents documents) {
        if (guard == nullptr || generation != guard->m_runGeneration || !guard->isVisible()) {
            return;
        }
        guard->clearConsole();
        guard->m_preview->render(documents);
        guard->m_outputTabs->setCurrentWidget(guard->m_preview);
    });
}

void WebPlaygroundPanel::stopPreview()
{
    m_autoRunTimer->stop();
    ++m_runGeneration;
    if (m_preview != nullptr) {
        m_preview->stop();
    }
}

void WebPlaygroundPanel::resetDocuments()
{
    if (m_editor == nullptr) {
        return;
    }
    const quint64 generation = ++m_runGeneration;
    const QPointer<WebPlaygroundPanel> guard(this);
    m_editor->requestDocuments([guard, generation](const WebPlayground::Documents &documents) {
        if (guard == nullptr || generation != guard->m_runGeneration) {
            return;
        }
        const auto starter = WebPlayground::starterDocuments();
        if (documents != starter
            && QMessageBox::question(guard,
                                     guard->tr("Reset Playground"),
                                     guard->tr("Discard the current HTML, CSS, and JavaScript draft?"))
                   != QMessageBox::Yes) {
            return;
        }
        guard->m_editor->replaceDocuments(starter);
        guard->m_preview->clear();
        guard->clearConsole();
    });
}

void WebPlaygroundPanel::appendConsole(const QString &severity,
                                       const QString &message,
                                       int lineNumber,
                                       const QString &sourceId)
{
    QString displayedSeverity = severity;
    QString displayedMessage = message;
    if (m_consoleRateTimer.elapsed() >= 1000) {
        m_consoleRateTimer.restart();
        m_consoleMessagesThisSecond = 0;
    }
    ++m_consoleMessagesThisSecond;
    if (m_consoleMessagesThisSecond > 101) {
        return;
    }
    if (m_consoleMessagesThisSecond == 101) {
        displayedSeverity = QStringLiteral("Warning");
        displayedMessage = tr("Console output throttled.");
        lineNumber = 0;
    }

    while (m_console->count() >= 500) {
        delete m_console->takeItem(0);
    }
    QString source = QFileInfo(QUrl(sourceId).path()).fileName();
    if (source.isEmpty() && !sourceId.isEmpty()) {
        source = sourceId.right(48);
    }
    QString location;
    if (!source.isEmpty() || lineNumber > 0) {
        location = QStringLiteral(" [%1:%2]").arg(source, QString::number(lineNumber));
    }
    auto *item = new QListWidgetItem(QStringLiteral("%1  %2  %3%4")
                                         .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz")),
                                              displayedSeverity,
                                              displayedMessage,
                                              location),
                                     m_console);
    item->setData(Qt::UserRole, displayedSeverity);
    updateConsoleColors();
}

void WebPlaygroundPanel::clearConsole()
{
    m_console->clear();
}

void WebPlaygroundPanel::openInBrowser()
{
    const quint64 generation = ++m_runGeneration;
    const QPointer<WebPlaygroundPanel> guard(this);
    m_editor->requestDocuments([guard, generation](const WebPlayground::Documents &documents) {
        if (guard == nullptr || generation != guard->m_runGeneration) {
            return;
        }

        auto project = std::make_unique<QTemporaryDir>(QStringLiteral("%1/zealrn-playground-XXXXXX")
                                                            .arg(QDir::tempPath()));
        QString error;
        if (!project->isValid()
            || WebPlayground::writeProject(project->path(), documents, true, &error)
                   != WebPlayground::ExportResult::Success) {
            QMessageBox::warning(guard,
                                 guard->tr("Open in Browser"),
                                 error.isEmpty() ? guard->tr("Could not create a temporary project.") : error);
            return;
        }

        const QUrl indexUrl = QUrl::fromLocalFile(QDir(project->path()).filePath(QStringLiteral("index.html")));
        QMessageBox::information(
            guard,
            guard->tr("Open in Browser"),
            guard->tr("The external browser runs this project outside the isolated preview restrictions."));
        if (!QDesktopServices::openUrl(indexUrl)) {
            QMessageBox::warning(guard, guard->tr("Open in Browser"), guard->tr("Could not open the external browser."));
            return;
        }
        guard->m_externalProject = std::move(project);
    });
}

void WebPlaygroundPanel::exportProject()
{
    const QString parentDirectory = QFileDialog::getExistingDirectory(this,
                                                                       tr("Export Project"),
                                                                       QDir::homePath());
    if (parentDirectory.isEmpty()) {
        return;
    }

    bool accepted = false;
    const QString name = QInputDialog::getText(this,
                                                tr("Project Name"),
                                                tr("Directory name:"),
                                                QLineEdit::Normal,
                                                QStringLiteral("zealrn-project"),
                                                &accepted)
                             .trimmed();
    if (!accepted) {
        return;
    }
    if (name.isEmpty() || name == QStringLiteral(".") || name == QStringLiteral("..")
        || QFileInfo(name).fileName() != name || name.contains('/') || name.contains('\\')) {
        QMessageBox::warning(this, tr("Export Project"), tr("Enter a single valid directory name."));
        return;
    }

    const QString projectDirectory = QDir(parentDirectory).filePath(name);
    const quint64 generation = ++m_runGeneration;
    const QPointer<WebPlaygroundPanel> guard(this);
    m_editor->requestDocuments([guard, generation, projectDirectory](const WebPlayground::Documents &documents) {
        if (guard == nullptr || generation != guard->m_runGeneration) {
            return;
        }
        QString error;
        auto result = WebPlayground::writeProject(projectDirectory, documents, false, &error);
        if (result == WebPlayground::ExportResult::Exists) {
            if (QMessageBox::question(guard,
                                      guard->tr("Export Project"),
                                      guard->tr("Replace existing index.html, style.css, and script.js files?"))
                != QMessageBox::Yes) {
                return;
            }
            result = WebPlayground::writeProject(projectDirectory, documents, true, &error);
        }
        if (result != WebPlayground::ExportResult::Success) {
            QMessageBox::warning(guard,
                                 guard->tr("Export Project"),
                                 error.isEmpty() ? guard->tr("Could not export the project.") : error);
            return;
        }
        QMessageBox::information(guard,
                                 guard->tr("Export Project"),
                                 guard->tr("Project exported to %1").arg(QDir::toNativeSeparators(projectDirectory)));
    });
}

void WebPlaygroundPanel::updateConsoleColors()
{
    const QPalette currentPalette = palette();
    for (int i = 0; i < m_console->count(); ++i) {
        auto *item = m_console->item(i);
        const QString severity = item->data(Qt::UserRole).toString();
        QPalette::ColorRole role = QPalette::Text;
        if (severity == QStringLiteral("Error")) {
            role = QPalette::BrightText;
        } else if (severity == QStringLiteral("Warning")) {
            role = QPalette::LinkVisited;
        }
        item->setForeground(currentPalette.color(role));
    }
}

} // namespace Zeal::WidgetUi
