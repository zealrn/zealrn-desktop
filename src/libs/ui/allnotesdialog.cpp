// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "allnotesdialog.h"

#include "learningnotesstore.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace Zeal::WidgetUi {

AllNotesDialog::AllNotesDialog(LearningNotesStore *store, QWidget *parent)
    : QDialog(parent)
    , m_store(store)
{
    setWindowTitle(tr("All Learning Notes"));
    resize(820, 520);

    auto *layout = new QVBoxLayout(this);
    auto *filters = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search notes"));
    filters->addWidget(m_searchEdit, 1);
    m_docsetFilter = new QComboBox(this);
    m_docsetFilter->addItem(tr("All docsets"));
    filters->addWidget(m_docsetFilter);
    layout->addLayout(filters);

    auto *splitter = new QSplitter(this);
    m_list = new QListWidget(splitter);
    m_editor = new QPlainTextEdit(splitter);
    splitter->addWidget(m_list);
    splitter->addWidget(m_editor);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    layout->addWidget(m_errorLabel);

    auto *buttons = new QHBoxLayout();
    m_openButton = new QPushButton(tr("Open Documentation"), this);
    m_saveButton = new QPushButton(tr("Save"), this);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_exportButton = new QPushButton(tr("Export"), this);
    buttons->addWidget(m_openButton);
    buttons->addStretch();
    buttons->addWidget(m_saveButton);
    buttons->addWidget(m_deleteButton);
    buttons->addWidget(m_exportButton);
    layout->addLayout(buttons);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(250);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() { m_searchTimer->start(); });
    connect(m_searchTimer, &QTimer::timeout, this, &AllNotesDialog::refresh);
    connect(m_docsetFilter, &QComboBox::currentTextChanged, this, [this]() { refresh(); });
    connect(m_list, &QListWidget::currentRowChanged, this, &AllNotesDialog::showCurrentNote);
    connect(m_saveButton, &QPushButton::clicked, this, &AllNotesDialog::saveCurrent);
    connect(m_deleteButton, &QPushButton::clicked, this, &AllNotesDialog::deleteCurrent);
    connect(m_openButton, &QPushButton::clicked, this, [this]() {
        if (const LearningNote *note = currentNote()) {
            emit openDocumentationRequested(note->page);
            accept();
        }
    });
    connect(m_exportButton, &QPushButton::clicked, this, [this]() {
        if (const LearningNote *note = currentNote()) {
            emit exportRequested(*note);
        }
    });

    const QList<LearningNote> allNotes = m_store->search();
    QStringList docsets;
    for (const LearningNote &note : allNotes) {
        docsets.append(note.page.docsetName);
    }
    docsets.removeDuplicates();
    std::ranges::sort(docsets);
    m_docsetFilter->addItems(docsets);
    refresh();
}

void AllNotesDialog::refresh()
{
    const qint64 selectedId = currentNote() ? currentNote()->id : 0;
    const QString docset = m_docsetFilter->currentIndex() > 0 ? m_docsetFilter->currentText() : QString();
    m_notes = m_store->search(m_searchEdit->text(), docset);
    m_list->clear();
    for (const LearningNote &note : m_notes) {
        const QString preview = note.content.simplified().left(100);
        auto *item = new QListWidgetItem(
            QStringLiteral("%1\n%2 - %3\n%4").arg(note.page.pageTitle,
                                                  note.page.docsetName,
                                                  QLocale().toString(note.updatedAt.toLocalTime(), QLocale::ShortFormat),
                                                  preview),
            m_list);
        item->setData(Qt::UserRole, note.id);
        if (note.id == selectedId) {
            m_list->setCurrentItem(item);
        }
    }
    if (m_list->currentRow() < 0 && m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
    m_errorLabel->setText(m_store->lastError());
    showCurrentNote();
}

LearningNote *AllNotesDialog::currentNote()
{
    const QListWidgetItem *item = m_list->currentItem();
    if (item == nullptr) {
        return nullptr;
    }
    const qint64 id = item->data(Qt::UserRole).toLongLong();
    const auto it = std::ranges::find(m_notes, id, &LearningNote::id);
    return it == m_notes.end() ? nullptr : &*it;
}

void AllNotesDialog::showCurrentNote()
{
    const LearningNote *note = currentNote();
    const bool selected = note != nullptr;
    m_editor->setEnabled(selected);
    m_openButton->setEnabled(selected);
    m_saveButton->setEnabled(selected);
    m_deleteButton->setEnabled(selected);
    m_exportButton->setEnabled(selected);
    m_editor->setPlainText(selected ? note->content : QString());
}

void AllNotesDialog::saveCurrent()
{
    LearningNote *note = currentNote();
    if (note == nullptr) {
        return;
    }
    note->content = m_editor->toPlainText();
    if (!m_store->save(note)) {
        m_errorLabel->setText(m_store->lastError());
        return;
    }
    refresh();
}

void AllNotesDialog::deleteCurrent()
{
    LearningNote *note = currentNote();
    if (note == nullptr
        || QMessageBox::question(this, tr("Delete Note"), tr("Delete this note permanently?")) != QMessageBox::Yes) {
        return;
    }
    if (!m_store->remove(note->id)) {
        m_errorLabel->setText(m_store->lastError());
        return;
    }
    refresh();
}

} // namespace Zeal::WidgetUi
