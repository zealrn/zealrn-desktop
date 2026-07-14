// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "quicktourdialog.h"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

namespace {
QWizardPage *makePage(const QString &title, const QString &text)
{
    auto *result = new QWizardPage;
    result->setTitle(title);
    auto *layout = new QVBoxLayout(result);
    auto *label = new QLabel(text, result);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    layout->addWidget(label);
    layout->addStretch();
    return result;
}
} // namespace

QuickTourDialog::QuickTourDialog(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Quick Tour"));
    setWizardStyle(QWizard::ModernStyle);
    setMinimumSize(620, 430);
    setOption(QWizard::NoBackButtonOnStartPage);

    auto *welcome = makePage(tr("Welcome to ZealRN"),
                         tr("Read offline documentation, take page-linked notes, try web examples, and use your "
                            "local development tools from one study workspace."));
    m_doNotShowAgain = new QCheckBox(tr("Do not show automatically again"), welcome);
    m_doNotShowAgain->setChecked(true);
    welcome->layout()->addWidget(m_doNotShowAgain);
    addPage(welcome);

    auto *docsets = makePage(tr("Download Documentation"),
                         tr("Open Docset Library and download only the documentation you need. Installed docsets "
                            "remain available offline."));
    auto *docsetsButton = new QPushButton(tr("Open Docset Library"), docsets);
    docsetsButton->setObjectName(QStringLiteral("openDocsetLibraryButton"));
    docsetsButton->setToolTip(tr("Browse downloadable documentation"));
    docsets->layout()->addWidget(docsetsButton);
    connect(docsetsButton, &QPushButton::clicked, this, &QuickTourDialog::openDocsetLibraryRequested);
    addPage(docsets);

    addPage(makePage(tr("Start Note and Page Notes"),
                 tr("Start Note is always available for quick ideas. When you open documentation, Learning Notes "
                    "switches to a note linked to that page and autosaves your work.")));
    addPage(makePage(tr("Web Playground"),
                 tr("Edit HTML, CSS, and JavaScript, then run them in an isolated local preview. The playground "
                    "does not require an internet connection.")));
#ifdef Q_OS_WINDOWS
    addPage(makePage(tr("Developer Terminal"),
                 tr("On Windows, ZealRN opens an installed Windows Terminal, PowerShell, Command Prompt, or Git "
                    "Bash in the working directory you choose.")));
#else
    addPage(makePage(tr("Developer Terminal"),
                 tr("The Developer Terminal is a normal local shell with your user permissions. Commands can "
                    "modify or delete files, so review commands before running them.")));
#endif
    addPage(makePage(tr("Backups and Exports"),
                 tr("Export notes as Markdown, searchable PDF, JSON, or a ZIP archive. You can also back up the "
                    "SQLite notes database.")));

    auto *ready = makePage(tr("Ready"), tr("Start with documentation or write a Start Note immediately."));
    auto *startNoteButton = new QPushButton(tr("Open Start Note"), ready);
    startNoteButton->setObjectName(QStringLiteral("openStartNoteButton"));
    startNoteButton->setToolTip(tr("Open the always-available Start Note"));
    ready->layout()->addWidget(startNoteButton);
    connect(startNoteButton, &QPushButton::clicked, this, &QuickTourDialog::openStartNoteRequested);
    addPage(ready);
}

bool QuickTourDialog::doNotShowAutomatically() const
{
    return m_doNotShowAgain->isChecked();
}

} // namespace Zeal::WidgetUi
