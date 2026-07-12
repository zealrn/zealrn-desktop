// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotesstore.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace Zeal::WidgetUi {
namespace {

LearningNote noteFromQuery(const QSqlQuery &query)
{
    return {.id = query.value(0).toLongLong(),
            .page = {.docsetId = query.value(1).toString(),
                     .docsetName = query.value(2).toString(),
                     .pageKey = query.value(3).toString(),
                     .pagePath = query.value(4).toString(),
                     .pageUrl = query.value(5).toString(),
                     .pageTitle = query.value(6).toString()},
            .content = query.value(7).toString(),
            .createdAt = QDateTime::fromString(query.value(8).toString(), Qt::ISODateWithMs),
            .updatedAt = QDateTime::fromString(query.value(9).toString(), Qt::ISODateWithMs)};
}

QString likePattern(QString text)
{
    text.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    text.replace(QLatin1Char('%'), QStringLiteral("\\%"));
    text.replace(QLatin1Char('_'), QStringLiteral("\\_"));
    return QLatin1Char('%') + text + QLatin1Char('%');
}

constexpr auto SelectColumns = "id, docset_id, docset_name, page_key, page_path, page_url, page_title, "
                               "content, created_at, updated_at";

} // namespace

LearningNotePage LearningNotePage::fromUrl(const QString &docsetId,
                                           const QString &docsetName,
                                           const QUrl &baseUrl,
                                           const QUrl &url,
                                           const QString &pageTitle)
{
    if (docsetId.isEmpty() || !baseUrl.isValid() || !url.isValid() || baseUrl.scheme() != url.scheme()
        || baseUrl.host() != url.host() || baseUrl.port() != url.port()) {
        return {};
    }

    QString basePath = baseUrl.path();
    if (!basePath.endsWith(QLatin1Char('/'))) {
        basePath += QLatin1Char('/');
    }
    if (!url.path().startsWith(basePath)) {
        return {};
    }

    QString pagePath = QDir::cleanPath(url.path().mid(basePath.size()));
    while (pagePath.startsWith(QLatin1Char('/'))) {
        pagePath.remove(0, 1);
    }
    if (pagePath.isEmpty() || pagePath == QLatin1String(".") || pagePath.startsWith(QLatin1String("../"))) {
        return {};
    }

    QUrl readableUrl = url;
    readableUrl.setQuery(QString());
    readableUrl.setFragment({});
    return {.docsetId = docsetId,
            .docsetName = docsetName,
            .pageKey = pagePath,
            .pagePath = pagePath,
            .pageUrl = readableUrl.toString(),
            .pageTitle = pageTitle};
}

LearningNotesStore::LearningNotesStore(const QString &databasePath)
    : m_connectionName(QStringLiteral("learning-notes-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
    , m_databasePath(databasePath.isEmpty()
                         ? QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                               .filePath(QStringLiteral("learning-notes.sqlite"))
                         : databasePath)
    , m_database(QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName))
{
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) {
        setLastError(QStringLiteral("Cannot create notes data directory."));
        return;
    }

    m_database.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));
    m_database.setDatabaseName(m_databasePath);
    if (!m_database.open()) {
        setLastError(m_database.lastError().text());
        return;
    }
    migrate();
}

LearningNotesStore::~LearningNotesStore()
{
    if (m_database.isValid()) {
        m_database.close();
    }
    m_database = {};
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool LearningNotesStore::isOpen() const
{
    return m_database.isOpen() && m_lastError.isEmpty();
}

QString LearningNotesStore::lastError() const
{
    return m_lastError;
}

int LearningNotesStore::schemaVersion() const
{
    QSqlQuery query(m_database);
    return query.exec(QStringLiteral("PRAGMA user_version")) && query.next() ? query.value(0).toInt() : 0;
}

QString LearningNotesStore::databasePath() const
{
    return m_databasePath;
}

bool LearningNotesStore::migrate()
{
    if (schemaVersion() == 1) {
        return true;
    }
    if (schemaVersion() != 0 || !m_database.transaction()) {
        setLastError(QStringLiteral("Unsupported notes database schema."));
        return false;
    }

    QSqlQuery query(m_database);
    const QStringList statements = {
        QStringLiteral("CREATE TABLE notes ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       "docset_id TEXT NOT NULL, docset_name TEXT NOT NULL,"
                       "page_key TEXT NOT NULL, page_path TEXT NOT NULL,"
                       "page_url TEXT NOT NULL DEFAULT '', page_title TEXT NOT NULL DEFAULT '',"
                       "content TEXT NOT NULL DEFAULT '', created_at TEXT NOT NULL, updated_at TEXT NOT NULL,"
                       "UNIQUE(docset_id, page_key))"),
        QStringLiteral("CREATE INDEX notes_updated_at_idx ON notes(updated_at DESC)"),
        QStringLiteral("CREATE INDEX notes_docset_idx ON notes(docset_id)"),
        QStringLiteral("PRAGMA user_version = 1")};
    for (const QString &statement : statements) {
        if (!query.exec(statement)) {
            m_database.rollback();
            setLastError(query.lastError().text());
            return false;
        }
    }
    if (!m_database.commit()) {
        setLastError(m_database.lastError().text());
        return false;
    }
    return true;
}

bool LearningNotesStore::save(LearningNote *note)
{
    if (note == nullptr || !note->page.isValid()) {
        setLastError(QStringLiteral("No documentation page selected."));
        return false;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    const QString created = (note->createdAt.isValid() ? note->createdAt : now).toString(Qt::ISODateWithMs);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO notes (docset_id, docset_name, page_key, page_path, page_url, page_title, content, created_at, "
        "updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(docset_id, page_key) DO UPDATE SET docset_name=excluded.docset_name, "
        "page_path=excluded.page_path, page_url=excluded.page_url, page_title=excluded.page_title, "
        "content=excluded.content, updated_at=excluded.updated_at"));
    query.addBindValue(note->page.docsetId);
    query.addBindValue(note->page.docsetName);
    query.addBindValue(note->page.pageKey);
    query.addBindValue(note->page.pagePath);
    query.addBindValue(note->page.pageUrl);
    query.addBindValue(note->page.pageTitle);
    query.addBindValue(note->content);
    query.addBindValue(created);
    query.addBindValue(now.toString(Qt::ISODateWithMs));
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return false;
    }

    const auto saved = this->note(note->page.docsetId, note->page.pageKey);
    if (!saved) {
        return false;
    }
    *note = *saved;
    m_lastError.clear();
    return true;
}

std::optional<LearningNote> LearningNotesStore::note(const QString &docsetId, const QString &pageKey)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT %1 FROM notes WHERE docset_id = ? AND page_key = ?").arg(
        QLatin1String(SelectColumns)));
    query.addBindValue(docsetId);
    query.addBindValue(pageKey);
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return {};
    }
    m_lastError.clear();
    return query.next() ? std::optional<LearningNote>(noteFromQuery(query)) : std::nullopt;
}

QList<LearningNote> LearningNotesStore::search(const QString &text, const QString &docsetName)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("SELECT %1 FROM notes WHERE (? = '' OR docset_name = ?) AND "
                                 "(? = '' OR page_title LIKE ? ESCAPE '\\' "
                                 "OR docset_name LIKE ? ESCAPE '\\' OR page_path LIKE ? ESCAPE '\\' "
                                 "OR content LIKE ? ESCAPE '\\') ORDER BY updated_at DESC")
                      .arg(QLatin1String(SelectColumns)));
    const QString pattern = likePattern(text);
    const QString boundDocset = docsetName.isNull() ? QStringLiteral("") : docsetName;
    const QString boundText = text.isNull() ? QStringLiteral("") : text;
    query.addBindValue(boundDocset);
    query.addBindValue(boundDocset);
    query.addBindValue(boundText);
    for (int i = 0; i < 4; ++i) {
        query.addBindValue(pattern);
    }
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return {};
    }

    QList<LearningNote> notes;
    while (query.next()) {
        notes.append(noteFromQuery(query));
    }
    m_lastError.clear();
    return notes;
}

bool LearningNotesStore::remove(qint64 id)
{
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM notes WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec()) {
        setLastError(query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

bool LearningNotesStore::checkpoint()
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(FULL)"))) {
        setLastError(query.lastError().text());
        return false;
    }
    m_lastError.clear();
    return true;
}

void LearningNotesStore::setLastError(const QString &error)
{
    m_lastError = error;
}

} // namespace Zeal::WidgetUi
