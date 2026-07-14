// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../learningnotepage.h"
#include "../learningnotesstore.h"

#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>

using namespace Zeal::WidgetUi;

class LearningNotesStoreTest : public QObject
{
    Q_OBJECT

private slots:
    void pageIdentity_ignoresServerAndAnchor();
    void pageIdentity_keepsDocsetsSeparate();
    void startNote_hasReservedIdentity();
    void store_createsVersionedDatabase();
    void store_rollsBackFailedMigration();
    void store_savesLoadsSearchesAndDeletesUnicodeNote();
    void store_searchesEveryFieldAndFiltersDocset();
};

void LearningNotesStoreTest::pageIdentity_ignoresServerAndAnchor()
{
    const LearningNotePage first = LearningNotePage::fromUrl(QStringLiteral("Python_3"),
                                                             QStringLiteral("Python 3"),
                                                             QUrl(QStringLiteral("http://127.0.0.1:43121/Python_3/")),
                                                             QUrl(QStringLiteral(
                                                                 "http://127.0.0.1:43121/Python_3/library/os.html#os.chdir")),
                                                             QStringLiteral("os — Operating system interfaces"));
    const LearningNotePage restarted = LearningNotePage::fromUrl(QStringLiteral("Python_3"),
                                                                 QStringLiteral("Python 3"),
                                                                 QUrl(QStringLiteral("http://127.0.0.1:52999/Python_3/")),
                                                                 QUrl(QStringLiteral(
                                                                     "http://127.0.0.1:52999/Python_3/library/os.html?x=1#top")),
                                                                 QStringLiteral("os"));

    QVERIFY(first.isValid());
    QCOMPARE(first.pagePath, QStringLiteral("library/os.html"));
    QCOMPARE(first.pageKey, restarted.pageKey);
}

void LearningNotesStoreTest::pageIdentity_keepsDocsetsSeparate()
{
    const QUrl url(QStringLiteral("http://127.0.0.1:43121/docs/index.html"));
    const LearningNotePage one = LearningNotePage::fromUrl(QStringLiteral("One"),
                                                           QStringLiteral("One"),
                                                           QUrl(QStringLiteral("http://127.0.0.1:43121/docs/")),
                                                           url,
                                                           QStringLiteral("Index"));
    const LearningNotePage two = LearningNotePage::fromUrl(QStringLiteral("Two"),
                                                           QStringLiteral("Two"),
                                                           QUrl(QStringLiteral("http://127.0.0.1:43121/docs/")),
                                                           url,
                                                           QStringLiteral("Index"));

    QCOMPARE(one.pageKey, two.pageKey);
    QVERIFY(one.docsetId != two.docsetId);
}

void LearningNotesStoreTest::startNote_hasReservedIdentity()
{
    const LearningNotePage start = LearningNotePage::startNote();

    QVERIFY(start.isValid());
    QVERIFY(start.isStartNote());
    QCOMPARE(start.docsetId, QStringLiteral("__zealrn_system__"));
    QCOMPARE(start.pageKey, QStringLiteral("start-note"));
    QVERIFY(start.pageUrl.isEmpty());
    QVERIFY(start.pagePath.isEmpty());

    QTemporaryDir dir;
    LearningNotesStore store(dir.filePath(QStringLiteral("notes.sqlite")));
    LearningNote startNote;
    startNote.page = start;
    startNote.content = QStringLiteral("Start");
    LearningNote docsetNote;
    docsetNote.page = {.docsetId = QStringLiteral("Docs"),
                       .docsetName = QStringLiteral("Docs"),
                       .pageKey = QStringLiteral("start-note"),
                       .pagePath = QStringLiteral("start-note"),
                       .pageUrl = QStringLiteral(""),
                       .pageTitle = QStringLiteral("A documentation page")};
    docsetNote.content = QStringLiteral("Docs");
    QVERIFY(store.save(&startNote));
    QVERIFY(store.save(&docsetNote));
    QCOMPARE(store.search().size(), 2);
}

void LearningNotesStoreTest::store_createsVersionedDatabase()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LearningNotesStore store(dir.filePath(QStringLiteral("notes.sqlite")));

    QVERIFY2(store.isOpen(), qPrintable(store.lastError()));
    QCOMPARE(store.schemaVersion(), 1);
}

void LearningNotesStoreTest::store_rollsBackFailedMigration()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("notes.sqlite"));
    const QString connection = QStringLiteral("migration-fixture");
    {
        QSqlDatabase database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        database.setDatabaseName(path);
        QVERIFY(database.open());
        QSqlQuery query(database);
        QVERIFY(query.exec(QStringLiteral("CREATE TABLE notes (sentinel TEXT)")));
        database.close();
    }
    QSqlDatabase::removeDatabase(connection);

    LearningNotesStore store(path);
    QVERIFY(!store.isOpen());
    QCOMPARE(store.schemaVersion(), 0);
    QVERIFY(!store.lastError().isEmpty());
}

void LearningNotesStoreTest::store_savesLoadsSearchesAndDeletesUnicodeNote()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    LearningNotesStore store(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY2(store.isOpen(), qPrintable(store.lastError()));

    LearningNote note;
    note.page = LearningNotePage::fromUrl(QStringLiteral("Qt_6"),
                                          QStringLiteral("Qt 6"),
                                          QUrl(QStringLiteral("http://localhost:3000/Qt_6/")),
                                          QUrl(QStringLiteral("http://localhost:3000/Qt_6/qsqlquery.html")),
                                          QStringLiteral("QSqlQuery"));
    note.content = QStringLiteral("Prepared statements: 参数绑定 ✓");

    QVERIFY2(store.save(&note), qPrintable(store.lastError()));
    QVERIFY(note.id > 0);
    QVERIFY(note.createdAt.isValid());
    QVERIFY(note.updatedAt.isValid());

    const auto loaded = store.note(note.page.docsetId, note.page.pageKey);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->content, note.content);
    QCOMPARE(store.search(QStringLiteral("参数")).size(), 1);
    QCOMPARE(store.search(QStringLiteral("Qt 6")).size(), 1);

    note.content = QStringLiteral("Updated");
    QVERIFY(store.save(&note));
    QCOMPARE(store.search(QString()).size(), 1);

    QVERIFY(store.remove(note.id));
    QVERIFY(!store.note(note.page.docsetId, note.page.pageKey).has_value());
}

void LearningNotesStoreTest::store_searchesEveryFieldAndFiltersDocset()
{
    QTemporaryDir dir;
    LearningNotesStore store(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(store.isOpen());

    LearningNote qt;
    qt.page = {.docsetId = QStringLiteral("Qt_6"),
               .docsetName = QStringLiteral("Qt 6"),
               .pageKey = QStringLiteral("sql/query.html"),
               .pagePath = QStringLiteral("sql/query.html"),
               .pageUrl = QStringLiteral("http://localhost/Qt_6/sql/query.html"),
               .pageTitle = QStringLiteral("Prepared Queries")};
    qt.content = QStringLiteral("Bind values");
    QVERIFY(store.save(&qt));

    LearningNote git;
    git.page = {.docsetId = QStringLiteral("Git"),
                .docsetName = QStringLiteral("Git"),
                .pageKey = QStringLiteral("query.html"),
                .pagePath = QStringLiteral("query.html"),
                .pageUrl = QStringLiteral("http://localhost/Git/query.html"),
                .pageTitle = QStringLiteral("Query")};
    git.content = QStringLiteral("Different content");
    QVERIFY(store.save(&git));

    QCOMPARE(store.search(QStringLiteral("Prepared")).size(), 1);
    QCOMPARE(store.search(QStringLiteral("sql/query")).size(), 1);
    QCOMPARE(store.search(QStringLiteral("Bind")).size(), 1);
    QCOMPARE(store.search(QString(), QStringLiteral("Git")).size(), 1);
    QCOMPARE(store.search(QStringLiteral("Query"), QStringLiteral("Qt 6")).size(), 1);
}

QTEST_MAIN(LearningNotesStoreTest)
#include "learningnotesstore_test.moc"
