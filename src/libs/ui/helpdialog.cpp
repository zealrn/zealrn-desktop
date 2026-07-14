// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "helpdialog.h"

#include <core/application.h>

#include <QClipboard>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSysInfo>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

namespace {
QTextBrowser *helpPage(const QString &markdown, QWidget *parent)
{
    auto *browser = new QTextBrowser(parent);
    browser->setMarkdown(markdown);
    browser->setOpenLinks(false);
    browser->setOpenExternalLinks(false);
    return browser;
}
} // namespace

HelpDialog::HelpDialog(Section section, const QString &terminalBackend, int schemaVersion, int docsetCount,
                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("ZealRN Help"));
    resize(720, 560);
    auto *layout = new QVBoxLayout(this);
    m_tabs = new QTabWidget(this);
    layout->addWidget(m_tabs);

    m_tabs->addTab(helpPage(tr("# Getting Started\n\n"
                               "1. Open **File > Docset Library** to download offline documentation.\n"
                               "2. Use **Start Note** for ideas that are not tied to a page.\n"
                               "3. Open a documentation page to create a page-linked note.\n"
                               "4. Use **View** to open Web Playground or Developer Terminal.\n\n"
                               "Notes autosave locally. Export backups regularly."),
                             m_tabs),
                   tr("Getting Started"));
    m_tabs->addTab(helpPage(keyboardShortcutsText(), m_tabs), tr("Keyboard Shortcuts"));
    m_tabs->addTab(helpPage(tr("# Notes and Backups\n\n"
                               "**Start Note** is always available. Page notes belong to one documentation page.\n\n"
                               "Use Export for Markdown, searchable PDF, JSON, ZIP, or a database backup. Notes "
                               "remain in your local ZealRN data directory and are not uploaded."),
                             m_tabs),
                   tr("Notes and Backups"));
    m_tabs->addTab(helpPage(tr("# Web Playground\n\n"
                               "Edit HTML, CSS, and JavaScript, then choose **Run**. The internal preview blocks "
                               "external network requests. Exported projects run outside those restrictions."),
                             m_tabs),
                   tr("Web Playground"));
#ifdef Q_OS_WINDOWS
    const QString terminalHelp = tr("# Developer Terminal\n\n"
                                    "This release opens an installed Windows Terminal, PowerShell, Command Prompt, "
                                    "or Git Bash in your selected directory. Commands have your normal user access.");
#else
    const QString terminalHelp = tr("# Developer Terminal\n\n"
                                    "The embedded terminal is a normal local shell with your user permissions. "
                                    "Commands can modify or delete files. ZealRN does not sandbox terminal commands.");
#endif
    m_tabs->addTab(helpPage(terminalHelp, m_tabs), tr("Developer Terminal"));
    m_tabs->addTab(helpPage(tr("# Docset Library\n\n"
                               "Open **File > Docset Library**, choose **Available**, and refresh the catalog. "
                               "Downloaded docsets are stored locally and continue working offline."),
                             m_tabs),
                   tr("Docset Library"));

    auto *report = new QWidget(m_tabs);
    auto *reportLayout = new QVBoxLayout(report);
    auto *description = new QLabel(tr("Copy the diagnostics below, then open a ZealRN issue. The report excludes "
                                      "note text, usernames, secrets, and file paths."),
                                   report);
    description->setWordWrap(true);
    reportLayout->addWidget(description);
    auto *diagnostics = new QPlainTextEdit(report);
    diagnostics->setReadOnly(true);
    diagnostics->setPlainText(diagnosticReport(terminalBackend, schemaVersion, docsetCount));
    reportLayout->addWidget(diagnostics);
    auto *copyButton = new QPushButton(tr("Copy Diagnostics"), report);
    auto *reportButton = new QPushButton(tr("Report a Problem"), report);
    reportLayout->addWidget(copyButton);
    reportLayout->addWidget(reportButton);
    connect(copyButton, &QPushButton::clicked, this, [diagnostics]() {
        QGuiApplication::clipboard()->setText(diagnostics->toPlainText());
    });
    connect(reportButton, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(reportIssueUrl());
    });
    m_tabs->addTab(report, tr("Report a Problem"));

    m_tabs->setCurrentIndex(static_cast<int>(section));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QUrl HelpDialog::reportIssueUrl()
{
    return QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn/issues/new"));
}

QString HelpDialog::diagnosticReport(const QString &terminalBackend, int schemaVersion, int docsetCount)
{
    return QStringLiteral("ZealRN %1\nOperating system: %2\nQt: %3\nTerminal backend: %4\nSchema version: %5\nDocsets: %6")
        .arg(Core::Application::version().toString(),
             QSysInfo::prettyProductName(),
             QString::fromLatin1(qVersion()),
             terminalBackend,
             QString::number(schemaVersion),
             QString::number(docsetCount));
}

QString HelpDialog::keyboardShortcutsText()
{
    return tr("# Keyboard Shortcuts\n\n"
              "- **Ctrl+S** — Save the current note\n"
              "- **Ctrl++ / Ctrl+- / Ctrl+0** — Change or reset note zoom\n"
              "- **Ctrl+F** — Find in the focused note or documentation page\n"
              "- **Ctrl+`** — Show Developer Terminal\n"
              "- **Ctrl+Shift+L** — Open Docset Library\n"
              "- **Ctrl+B** — Toggle the documentation sidebar\n"
              "- **Ctrl+K / Ctrl+L** — Focus documentation search\n"
              "- **Ctrl+Shift+7 / 8 / 9** — Numbered list, bullet list, or blockquote in notes\n"
              "- **Alt+Left / Alt+Right** — Navigate back or forward");
}

} // namespace Zeal::WidgetUi
