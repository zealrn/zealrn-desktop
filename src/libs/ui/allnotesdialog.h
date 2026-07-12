// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_ALLNOTESDIALOG_H
#define ZEAL_WIDGETUI_ALLNOTESDIALOG_H

#include "learningnotepage.h"
#include "learningnotesexporter.h"

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class QToolButton;

namespace Zeal::WidgetUi {

class LearningNotesStore;

class AllNotesDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AllNotesDialog)
public:
    explicit AllNotesDialog(LearningNotesStore *store, QWidget *parent = nullptr);

signals:
    void openDocumentationRequested(const LearningNotePage &page);
    void exportRequested(const LearningNote &note, LearningNotesExport::Format format);

private:
    void refresh();
    void showCurrentNote();
    LearningNote *currentNote();
    void saveCurrent();
    void deleteCurrent();

    LearningNotesStore *m_store = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_docsetFilter = nullptr;
    QListWidget *m_list = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QLabel *m_errorLabel = nullptr;
    QPushButton *m_openButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QToolButton *m_exportButton = nullptr;
    QTimer *m_searchTimer = nullptr;
    QList<LearningNote> m_notes;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_ALLNOTESDIALOG_H
