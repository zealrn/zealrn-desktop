// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESEXPORTER_H
#define ZEAL_WIDGETUI_LEARNINGNOTESEXPORTER_H

#include "learningnotepage.h"

namespace Zeal::WidgetUi::LearningNotesExport {

enum class Result {
    Success,
    Exists,
    Error
};

enum class Format {
    Markdown,
    Pdf,
    Json
};

QString safeFileName(const QString &docsetName, const QString &pageTitle);
QByteArray markdown(const LearningNote &note);
QByteArray json(const LearningNote &note);

Result writeMarkdown(const QString &path, const LearningNote &note, bool overwrite, QString *error = nullptr);
Result writeJson(const QString &path, const LearningNote &note, bool overwrite, QString *error = nullptr);
Result writePdf(const QString &path, const LearningNote &note, bool overwrite, QString *error = nullptr);
Result writeAllZip(const QString &path,
                   const QList<LearningNote> &notes,
                   bool overwrite,
                   QString *error = nullptr);
Result copyDatabase(const QString &source, const QString &destination, bool overwrite, QString *error = nullptr);

} // namespace Zeal::WidgetUi::LearningNotesExport

#endif // ZEAL_WIDGETUI_LEARNINGNOTESEXPORTER_H
