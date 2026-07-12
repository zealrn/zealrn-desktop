// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESSTORE_H
#define ZEAL_WIDGETUI_LEARNINGNOTESSTORE_H

#include "learningnotepage.h"

#include <QSqlDatabase>

#include <optional>

namespace Zeal::WidgetUi {

class LearningNotesStore
{
public:
    explicit LearningNotesStore(const QString &databasePath = {});
    ~LearningNotesStore();

    LearningNotesStore(const LearningNotesStore &) = delete;
    LearningNotesStore &operator=(const LearningNotesStore &) = delete;

    bool isOpen() const;
    QString lastError() const;
    int schemaVersion() const;
    QString databasePath() const;

    bool save(LearningNote *note);
    std::optional<LearningNote> note(const QString &docsetId, const QString &pageKey);
    QList<LearningNote> search(const QString &text = {}, const QString &docsetName = {});
    bool remove(qint64 id);

private:
    bool migrate();
    void setLastError(const QString &error);

    QString m_connectionName;
    QString m_databasePath;
    QSqlDatabase m_database;
    QString m_lastError;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_LEARNINGNOTESSTORE_H
