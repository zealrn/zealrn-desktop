// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../learningnotesexporter.h"
#include "../learningnotesstore.h"

#include <archive.h>
#include <archive_entry.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>

using namespace Zeal::WidgetUi;
using namespace Zeal::WidgetUi::LearningNotesExport;

class LearningNotesExporterTest : public QObject
{
    Q_OBJECT

private slots:
    void safeFileName_removesInvalidCharacters();
    void markdownAndJson_preserveUnicodeAndIdentity();
    void writers_refuseOverwrite();
    void pdf_containsSearchableText();
    void zip_containsIndexAndUniqueMarkdownFiles();
    void databaseBackup_isReadable();
};

static LearningNote note(QString title = QStringLiteral("QFile / QSaveFile"))
{
    LearningNote value;
    value.id = 7;
    value.page = {.docsetId = QStringLiteral("Qt_6"),
                  .docsetName = QStringLiteral("Qt 6"),
                  .pageKey = QStringLiteral("qsavefile.html"),
                  .pagePath = QStringLiteral("qsavefile.html"),
                  .pageUrl = QStringLiteral("http://localhost/Qt_6/qsavefile.html"),
                  .pageTitle = std::move(title)};
    value.content = QStringLiteral("Atomic UTF-8: café 参数");
    value.createdAt = QDateTime::fromString(QStringLiteral("2026-07-13T10:00:00.000Z"), Qt::ISODateWithMs);
    value.updatedAt = QDateTime::fromString(QStringLiteral("2026-07-13T11:00:00.000Z"), Qt::ISODateWithMs);
    return value;
}

void LearningNotesExporterTest::safeFileName_removesInvalidCharacters()
{
    QCOMPARE(safeFileName(QStringLiteral("Qt 6"), QStringLiteral("A/B:C*D?")), QStringLiteral("Qt 6 - A_B_C_D_"));
}

void LearningNotesExporterTest::markdownAndJson_preserveUnicodeAndIdentity()
{
    const LearningNote value = note();
    const QByteArray markdownData = markdown(value);
    QVERIFY(markdownData.contains("# QFile / QSaveFile"));
    QVERIFY(markdownData.contains(QStringLiteral("café 参数").toUtf8()));

    const QJsonObject object = QJsonDocument::fromJson(json(value)).object();
    QCOMPARE(object.value(QStringLiteral("format_version")).toInt(), 1);
    QCOMPARE(object.value(QStringLiteral("docset_id")).toString(), QStringLiteral("Qt_6"));
    QCOMPARE(object.value(QStringLiteral("content")).toString(), value.content);
}

void LearningNotesExporterTest::writers_refuseOverwrite()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("note.md"));
    QCOMPARE(writeMarkdown(path, note(), false), Result::Success);
    QCOMPARE(writeMarkdown(path, note(), false), Result::Exists);
}

void LearningNotesExporterTest::pdf_containsSearchableText()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("note.pdf"));
    QCOMPARE(writePdf(path, note(), false), Result::Success);
    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.read(4) == QByteArrayLiteral("%PDF"));
}

void LearningNotesExporterTest::zip_containsIndexAndUniqueMarkdownFiles()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("notes.zip"));
    const QList<LearningNote> notes = {note(QStringLiteral("Same")), note(QStringLiteral("Same"))};
    QCOMPARE(writeAllZip(path, notes, false), Result::Success);

    archive *reader = archive_read_new();
    archive_read_support_format_zip(reader);
    QCOMPARE(archive_read_open_filename(reader, path.toLocal8Bit().constData(), 10240), ARCHIVE_OK);
    QStringList entries;
    archive_entry *entry = nullptr;
    while (archive_read_next_header(reader, &entry) == ARCHIVE_OK) {
        entries.append(QString::fromUtf8(archive_entry_pathname(entry)));
        archive_read_data_skip(reader);
    }
    archive_read_free(reader);

    QVERIFY(std::ranges::any_of(entries, [](const QString &entryName) { return entryName.endsWith("/README.md"); }));
    QVERIFY(std::ranges::any_of(entries, [](const QString &entryName) { return entryName.endsWith("/index.json"); }));
    QCOMPARE(std::ranges::count_if(entries,
                                   [](const QString &entryName) { return entryName.contains("/notes/"); }),
             2);
    QVERIFY(std::ranges::any_of(entries, [](const QString &entryName) { return entryName.endsWith("-2.md"); }));
}

void LearningNotesExporterTest::databaseBackup_isReadable()
{
    QTemporaryDir dir;
    const QString source = dir.filePath(QStringLiteral("source.sqlite"));
    const QString backup = dir.filePath(QStringLiteral("backup.sqlite"));
    {
        LearningNotesStore store(source);
        LearningNote value = note();
        QVERIFY(store.save(&value));
        QVERIFY(store.checkpoint());
        QCOMPARE(copyDatabase(source, backup, false), Result::Success);
    }

    LearningNotesStore restored(backup);
    const auto value = restored.note(QStringLiteral("Qt_6"), QStringLiteral("qsavefile.html"));
    QVERIFY(value.has_value());
    QCOMPARE(value->content, QStringLiteral("Atomic UTF-8: café 参数"));
}

QTEST_MAIN(LearningNotesExporterTest)
#include "learningnotesexporter_test.moc"
