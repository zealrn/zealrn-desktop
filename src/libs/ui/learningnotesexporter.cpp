// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotesexporter.h"

#include <archive.h>
#include <archive_entry.h>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPrinter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextDocument>

namespace Zeal::WidgetUi::LearningNotesExport {
namespace {

QJsonObject noteObject(const LearningNote &note)
{
    return {{QStringLiteral("format_version"), 1},
            {QStringLiteral("zealrn_version"), QCoreApplication::applicationVersion()},
            {QStringLiteral("note_id"), note.id},
            {QStringLiteral("docset_id"), note.page.docsetId},
            {QStringLiteral("docset_name"), note.page.docsetName},
            {QStringLiteral("page_key"), note.page.pageKey},
            {QStringLiteral("page_path"), note.page.pagePath},
            {QStringLiteral("page_url"), note.page.pageUrl},
            {QStringLiteral("page_title"), note.page.pageTitle},
            {QStringLiteral("content"), note.content},
            {QStringLiteral("created_at"), note.createdAt.toUTC().toString(Qt::ISODateWithMs)},
            {QStringLiteral("updated_at"), note.updatedAt.toUTC().toString(Qt::ISODateWithMs)}};
}

Result writeAtomic(const QString &path, const QByteArray &data, bool overwrite, QString *error)
{
    if (!overwrite && QFileInfo::exists(path)) {
        return Result::Exists;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return Result::Error;
    }
    return Result::Success;
}

Result finalizeTemporaryFile(const QString &temporaryPath,
                             const QString &destination,
                             bool overwrite,
                             QString *error)
{
    QFile temporary(temporaryPath);
    if (!temporary.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = temporary.errorString();
        }
        QFile::remove(temporaryPath);
        return Result::Error;
    }
    const Result result = writeAtomic(destination, temporary.readAll(), overwrite, error);
    temporary.close();
    QFile::remove(temporaryPath);
    return result;
}

bool addArchiveFile(archive *writer, const QString &path, const QByteArray &data, QString *error)
{
    archive_entry *entry = archive_entry_new();
    const QByteArray encodedPath = path.toUtf8();
    archive_entry_set_pathname(entry, encodedPath.constData());
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, data.size());
    const bool ok = archive_write_header(writer, entry) == ARCHIVE_OK
                 && archive_write_data(writer, data.constData(), data.size()) == data.size();
    if (!ok && error != nullptr) {
        *error = QString::fromUtf8(archive_error_string(writer));
    }
    archive_entry_free(entry);
    return ok;
}

} // namespace

QString safeFileName(const QString &docsetName, const QString &pageTitle)
{
    QString name = QStringLiteral("%1 - %2").arg(docsetName, pageTitle).simplified();
    name.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
    while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' '))) {
        name.chop(1);
    }
    if (name.isEmpty()) {
        name = QStringLiteral("note");
    }
    return name.left(120);
}

QByteArray markdown(const LearningNote &note)
{
    return QStringLiteral("# %1\n\n"
                          "- Docset: %2\n"
                          "- Documentation: %3\n"
                          "- Created: %4\n"
                          "- Updated: %5\n\n"
                          "%6\n")
        .arg(note.page.pageTitle,
             note.page.docsetName,
             note.page.pagePath,
             note.createdAt.toLocalTime().toString(Qt::ISODate),
             note.updatedAt.toLocalTime().toString(Qt::ISODate),
             note.content)
        .toUtf8();
}

QByteArray json(const LearningNote &note)
{
    return QJsonDocument(noteObject(note)).toJson(QJsonDocument::Indented);
}

Result writeMarkdown(const QString &path, const LearningNote &note, bool overwrite, QString *error)
{
    return writeAtomic(path, markdown(note), overwrite, error);
}

Result writeJson(const QString &path, const LearningNote &note, bool overwrite, QString *error)
{
    return writeAtomic(path, json(note), overwrite, error);
}

Result writePdf(const QString &path, const LearningNote &note, bool overwrite, QString *error)
{
    if (!overwrite && QFileInfo::exists(path)) {
        return Result::Exists;
    }
    QTemporaryFile temporary(QFileInfo(path).absolutePath() + QStringLiteral("/.zealrn-note-XXXXXX.pdf"));
    temporary.setAutoRemove(false);
    if (!temporary.open()) {
        if (error != nullptr) {
            *error = temporary.errorString();
        }
        return Result::Error;
    }
    const QString temporaryPath = temporary.fileName();
    temporary.close();

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(temporaryPath);
    printer.setDocName(note.page.pageTitle);
    QTextDocument document;
    document.setPlainText(QStringLiteral("ZealRN Learning Note\n\n%1\n%2\n%3\n\nExported: %4\n\n%5")
                              .arg(note.page.pageTitle,
                                   note.page.docsetName,
                                   note.page.pagePath,
                                   QDateTime::currentDateTime().toString(Qt::ISODate),
                                   note.content));
    document.print(&printer);
    if (!QFileInfo(temporaryPath).exists() || QFileInfo(temporaryPath).size() == 0) {
        if (error != nullptr) {
            *error = QCoreApplication::translate("LearningNotesExport", "PDF generation failed.");
        }
        QFile::remove(temporaryPath);
        return Result::Error;
    }
    return finalizeTemporaryFile(temporaryPath, path, overwrite, error);
}

Result writeAllZip(const QString &path, const QList<LearningNote> &notes, bool overwrite, QString *error)
{
    if (!overwrite && QFileInfo::exists(path)) {
        return Result::Exists;
    }
    QTemporaryFile temporary(QFileInfo(path).absolutePath() + QStringLiteral("/.zealrn-notes-XXXXXX.zip"));
    temporary.setAutoRemove(false);
    if (!temporary.open()) {
        if (error != nullptr) {
            *error = temporary.errorString();
        }
        return Result::Error;
    }
    const QString temporaryPath = temporary.fileName();
    temporary.close();

    archive *writer = archive_write_new();
    archive_write_set_format_zip(writer);
    if (archive_write_open_filename(writer, temporaryPath.toLocal8Bit().constData()) != ARCHIVE_OK) {
        if (error != nullptr) {
            *error = QString::fromUtf8(archive_error_string(writer));
        }
        archive_write_free(writer);
        QFile::remove(temporaryPath);
        return Result::Error;
    }

    const QString root = QStringLiteral("notes-export-%1").arg(QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss")));
    QJsonArray index;
    QHash<QString, int> filenameCounts;
    bool ok = addArchiveFile(writer,
                             root + QStringLiteral("/README.md"),
                             QByteArrayLiteral("# ZealRN Learning Notes Export\n\nOne Markdown file is included per note.\n"),
                             error);
    for (const LearningNote &note : notes) {
        index.append(noteObject(note));
        const QString base = safeFileName(note.page.docsetName, note.page.pageTitle);
        const int count = ++filenameCounts[base];
        const QString suffix = count == 1 ? QString() : QStringLiteral("-%1").arg(count);
        ok = ok && addArchiveFile(writer,
                                  root + QStringLiteral("/notes/") + base + suffix + QStringLiteral(".md"),
                                  markdown(note),
                                  error);
    }
    const QJsonObject indexObject{{QStringLiteral("format_version"), 1},
                                  {QStringLiteral("zealrn_version"), QCoreApplication::applicationVersion()},
                                  {QStringLiteral("notes"), index}};
    ok = ok && addArchiveFile(writer,
                              root + QStringLiteral("/index.json"),
                              QJsonDocument(indexObject).toJson(QJsonDocument::Indented),
                              error);
    ok = archive_write_close(writer) == ARCHIVE_OK && ok;
    archive_write_free(writer);
    if (!ok) {
        QFile::remove(temporaryPath);
        return Result::Error;
    }
    return finalizeTemporaryFile(temporaryPath, path, overwrite, error);
}

Result copyDatabase(const QString &source, const QString &destination, bool overwrite, QString *error)
{
    QFile file(source);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return Result::Error;
    }
    return writeAtomic(destination, file.readAll(), overwrite, error);
}

} // namespace Zeal::WidgetUi::LearningNotesExport
