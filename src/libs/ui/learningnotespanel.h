// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
#define ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H

#include "learningnotepage.h"

#include <QWidget>

#include <memory>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;

namespace Zeal::WidgetUi {

class LearningNotesStore;

class LearningNotesPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(LearningNotesPanel)
public:
    explicit LearningNotesPanel(QWidget *parent = nullptr);
    explicit LearningNotesPanel(const QString &databasePath, QWidget *parent = nullptr);
    ~LearningNotesPanel() override;

    bool setPage(const LearningNotePage &page);
    bool flush();
    void appendSelection(const QString &selection);

signals:
    void addSelectionRequested();
    void allNotesRequested();

private:
    void setupUi();
    bool save(bool explicitSave);
    void setStatus(const QString &status);

    std::unique_ptr<LearningNotesStore> m_store;
    LearningNote m_note;
    QLabel *m_docsetLabel = nullptr;
    QLabel *m_pageLabel = nullptr;
    QLabel *m_pathLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_addSelectionButton = nullptr;
    QTimer *m_autoSaveTimer = nullptr;
    QString m_lastSelection;
    bool m_dirty = false;
    bool m_loading = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
