// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotespanel.h"

#include "allnotesdialog.h"
#include "learningnotesstore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFont>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

QString exportDirectory()
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return directory.isEmpty() ? QDir::homePath() : directory;
}

bool confirmOverwrite(QWidget *parent, const QString &path)
{
    return !QFileInfo::exists(path)
        || QMessageBox::question(parent,
                                 QCoreApplication::translate("LearningNotesPanel", "Replace File"),
                                 QCoreApplication::translate("LearningNotesPanel", "Replace the existing file?"))
               == QMessageBox::Yes;
}

} // namespace

namespace Zeal::WidgetUi {

LearningNotesPanel::LearningNotesPanel(QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>())
{
    setupUi();
}

LearningNotesPanel::LearningNotesPanel(const QString &databasePath, QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>(databasePath))
{
    setupUi();
}

LearningNotesPanel::~LearningNotesPanel()
{
    flush();
}

void LearningNotesPanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(QCoreApplication::translate("LearningNotesPanel", "Learning Notes"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    m_docsetLabel = new QLabel(this);
    m_docsetLabel->setWordWrap(true);
    layout->addWidget(m_docsetLabel);

    m_pageLabel = new QLabel(this);
    m_pageLabel->setWordWrap(true);
    layout->addWidget(m_pageLabel);

    m_pathLabel = new QLabel(this);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_pathLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("noteStatus"));
    layout->addWidget(m_statusLabel);

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("noteEditor"));
    m_editor->setPlaceholderText(
        QCoreApplication::translate("LearningNotesPanel", "Write what you learned from this documentation page…"));
    layout->addWidget(m_editor, 1);

    auto *buttonLayout = new QGridLayout();
    m_saveButton = new QPushButton(QCoreApplication::translate("LearningNotesPanel", "Save Note"), this);
    buttonLayout->addWidget(m_saveButton, 0, 0);

    m_addSelectionButton = new QPushButton(
        QCoreApplication::translate("LearningNotesPanel", "Add Selection"), this);
    buttonLayout->addWidget(m_addSelectionButton, 0, 1);

    auto *allNotesButton = new QPushButton(QCoreApplication::translate("LearningNotesPanel", "All Notes"), this);
    buttonLayout->addWidget(allNotesButton, 1, 0);

    m_exportButton = new QToolButton(this);
    m_exportButton->setText(QCoreApplication::translate("LearningNotesPanel", "Export"));
    m_exportButton->setPopupMode(QToolButton::InstantPopup);
    auto *exportMenu = new QMenu(m_exportButton);
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export Markdown"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Markdown);
    });
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export PDF"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Pdf);
    });
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export JSON"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Json);
    });
    exportMenu->addSeparator();
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export All Notes"),
                          this,
                          &LearningNotesPanel::exportAllNotes);
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Backup Database"),
                          this,
                          &LearningNotesPanel::backupDatabase);
    m_exportButton->setMenu(exportMenu);
    buttonLayout->addWidget(m_exportButton, 1, 1);
    layout->addLayout(buttonLayout);

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(1000);

    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        if (m_loading || !m_note.page.isValid()) {
            return;
        }
        m_dirty = true;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Unsaved"));
        m_autoSaveTimer->start();
    });
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() { save(false); });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() { save(true); });
    connect(m_addSelectionButton, &QPushButton::clicked, this, &LearningNotesPanel::addSelectionRequested);
    connect(allNotesButton, &QPushButton::clicked, this, &LearningNotesPanel::showAllNotes);
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() { save(true); });

    setPage({});
}

bool LearningNotesPanel::setPage(const LearningNotePage &page)
{
    if (m_note.page.docsetId == page.docsetId && m_note.page.pageKey == page.pageKey && page.isValid()) {
        m_note.page = page;
        m_docsetLabel->setText(page.docsetName);
        m_pageLabel->setText(page.pageTitle);
        m_pathLabel->setText(page.pagePath);
        return true;
    }
    if (!flush()) {
        return false;
    }

    m_autoSaveTimer->stop();
    m_note = {};
    m_note.page = page;
    m_lastSelection.clear();
    m_loading = true;
    m_editor->clear();
    m_loading = false;
    m_dirty = false;

    const bool valid = page.isValid();
    m_editor->setEnabled(valid);
    m_saveButton->setEnabled(valid);
    m_addSelectionButton->setEnabled(valid);
    m_exportButton->setEnabled(true);
    m_docsetLabel->setText(valid ? page.docsetName : QString());
    m_pageLabel->setText(valid ? page.pageTitle : QCoreApplication::translate(
                                                      "LearningNotesPanel", "No documentation page selected"));
    m_pathLabel->setText(valid ? page.pagePath : QString());
    if (!valid) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "No page"));
        return true;
    }

    if (const auto saved = m_store->note(page.docsetId, page.pageKey)) {
        m_note = *saved;
        m_loading = true;
        m_editor->setPlainText(m_note.content);
        m_loading = false;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Saved"));
    } else if (!m_store->lastError().isEmpty()) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Save failed"));
        m_editor->setToolTip(m_store->lastError());
    } else {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "New note"));
    }
    return true;
}

bool LearningNotesPanel::flush()
{
    m_autoSaveTimer->stop();
    return !m_dirty || save(false);
}

void LearningNotesPanel::appendSelection(const QString &selection)
{
    const QString trimmed = selection.trimmed();
    if (!m_note.page.isValid() || trimmed.isEmpty() || trimmed == m_lastSelection) {
        return;
    }

    QString quote = trimmed;
    quote.replace(QLatin1Char('\n'), QStringLiteral("\n> "));
    const QString separator = m_editor->toPlainText().isEmpty() ? QString() : QStringLiteral("\n\n");
    m_editor->moveCursor(QTextCursor::End);
    m_editor->insertPlainText(separator + QStringLiteral("> ") + quote);
    m_lastSelection = trimmed;
}

bool LearningNotesPanel::save(bool explicitSave)
{
    if (!m_note.page.isValid()) {
        return false;
    }
    m_note.content = m_editor->toPlainText();
    if (!explicitSave && m_note.id == 0 && m_note.content.trimmed().isEmpty()) {
        m_dirty = false;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "New note"));
        return true;
    }

    setStatus(QCoreApplication::translate("LearningNotesPanel", "Saving…"));
    if (!m_store->save(&m_note)) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Save failed"));
        m_editor->setToolTip(m_store->lastError());
        return false;
    }
    m_dirty = false;
    m_editor->setToolTip({});
    setStatus(QCoreApplication::translate("LearningNotesPanel", "Saved"));
    return true;
}

void LearningNotesPanel::showAllNotes()
{
    if (!flush()) {
        return;
    }
    AllNotesDialog dialog(m_store.get(), this);
    connect(&dialog, &AllNotesDialog::openDocumentationRequested, this, &LearningNotesPanel::openDocumentationRequested);
    connect(&dialog, &AllNotesDialog::exportRequested, this, &LearningNotesPanel::exportNote);
    dialog.exec();

    const LearningNotePage page = m_note.page;
    m_note = {};
    setPage(page);
}

void LearningNotesPanel::exportNote(const LearningNote &note, LearningNotesExport::Format format)
{
    if (!note.page.isValid()) {
        return;
    }
    const bool isCurrent = note.page.docsetId == m_note.page.docsetId && note.page.pageKey == m_note.page.pageKey;
    if (isCurrent && !flush()) {
        return;
    }
    const LearningNote value = isCurrent ? m_note : note;
    const QString base = LearningNotesExport::safeFileName(value.page.docsetName, value.page.pageTitle);
    const QString extension = format == LearningNotesExport::Format::Markdown
                                ? QStringLiteral("md")
                                : format == LearningNotesExport::Format::Pdf ? QStringLiteral("pdf")
                                                                            : QStringLiteral("json");
    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Export Learning Note"),
                                                      QDir(exportDirectory()).filePath(base + QLatin1Char('.')
                                                                                       + extension));
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }

    QString error;
    LearningNotesExport::Result result = LearningNotesExport::Result::Error;
    if (format == LearningNotesExport::Format::Markdown) {
        result = LearningNotesExport::writeMarkdown(path, value, true, &error);
    } else if (format == LearningNotesExport::Format::Pdf) {
        result = LearningNotesExport::writePdf(path, value, true, &error);
    } else {
        result = LearningNotesExport::writeJson(path, value, true, &error);
    }
    if (result == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Export Complete"), tr("The note was exported successfully."));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), error);
    }
}

void LearningNotesPanel::exportAllNotes()
{
    if (!flush()) {
        return;
    }
    const QList<LearningNote> notes = m_store->search();
    if (notes.isEmpty()) {
        QMessageBox::information(this, tr("Export All Notes"), tr("There are no saved notes to export."));
        return;
    }
    const QString suggested = QDir(exportDirectory())
                                  .filePath(QStringLiteral("notes-export-%1.zip").arg(
                                      QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    const QString path = QFileDialog::getSaveFileName(this, tr("Export All Notes"), suggested);
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }
    QString error;
    if (LearningNotesExport::writeAllZip(path, notes, true, &error) == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Export Complete"), tr("All notes were exported successfully."));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), error);
    }
}

void LearningNotesPanel::backupDatabase()
{
    if (!flush() || !m_store->checkpoint()) {
        QMessageBox::warning(this, tr("Backup Failed"), m_store->lastError());
        return;
    }
    const QString suggested = QDir(exportDirectory()).filePath(QStringLiteral("learning-notes-backup.sqlite"));
    const QString path = QFileDialog::getSaveFileName(this, tr("Backup Notes Database"), suggested);
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }
    QString error;
    if (LearningNotesExport::copyDatabase(m_store->databasePath(), path, true, &error)
        == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Backup Complete"), tr("The notes database was backed up successfully."));
    } else {
        QMessageBox::warning(this, tr("Backup Failed"), error);
    }
}

void LearningNotesPanel::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

} // namespace Zeal::WidgetUi
