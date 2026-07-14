// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
#define ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H

#include "learningnotepage.h"
#include "learningnotesexporter.h"

#include <QWidget>

#include <memory>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QTextBrowser;
class QTimer;
class QToolButton;

namespace Zeal::Core {
class Settings;
}

namespace Zeal::WidgetUi {

class LearningNotesStore;

class LearningNotesPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(LearningNotesPanel)
public:
    explicit LearningNotesPanel(QWidget *parent = nullptr);
    explicit LearningNotesPanel(Core::Settings *settings, QWidget *parent = nullptr);
    explicit LearningNotesPanel(const QString &databasePath, QWidget *parent = nullptr);
    LearningNotesPanel(const QString &databasePath, Core::Settings *settings, QWidget *parent = nullptr);
    ~LearningNotesPanel() override;

    bool setPage(const LearningNotePage &page);
    bool showStartNote();
    const LearningNotePage &currentPage() const;
    bool flush();
    void appendSelection(const QString &selection);
    void exitFocusMode();
    void exitExpandedMode();

signals:
    void addSelectionRequested();
    void openDocumentationRequested(const LearningNotePage &page);
    void expandedModeRequested(bool expanded);
    void focusModeRequested(bool focused);
    void noteSaved(const LearningNotePage &page);

private:
    void setupUi();
    void applyFormat(int action);
    void updatePreview();
    void updateCounts();
    void findNote(bool backward);
    void setZoom(int percent);
    void setLineWrap(bool enabled);
    bool save(bool explicitSave);
    void showAllNotes();
    void exportNote(const LearningNote &note, LearningNotesExport::Format format);
    void exportAllNotes();
    void backupDatabase();
    void clearCurrentNote();
    void setStatus(const QString &status);

    std::unique_ptr<LearningNotesStore> m_store;
    Core::Settings *m_settings = nullptr;
    LearningNote m_note;
    QLabel *m_docsetLabel = nullptr;
    QLabel *m_pageLabel = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_savedAtLabel = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_zoomLabel = nullptr;
    QLineEdit *m_findEdit = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QTextBrowser *m_preview = nullptr;
    QTabWidget *m_modeTabs = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_addSelectionButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QToolButton *m_exportButton = nullptr;
    QTimer *m_autoSaveTimer = nullptr;
    QTimer *m_previewTimer = nullptr;
    QAction *m_expandAction = nullptr;
    QAction *m_focusAction = nullptr;
    QString m_lastSelection;
    bool m_dirty = false;
    bool m_loading = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
