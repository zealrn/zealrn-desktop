// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundproject.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QUrl>

namespace Zeal::WidgetUi::WebPlayground {

Documents starterDocuments()
{
    return {
        QStringLiteral("<div class=\"card\">\n"
                       "  <h1>Hello ZealRN</h1>\n"
                       "  <p>Edit the code and click Run.</p>\n"
                       "  <button id=\"hello\">Click me</button>\n"
                       "</div>"),
        QStringLiteral("body {\n"
                       "  font-family: system-ui, sans-serif;\n"
                       "  padding: 2rem;\n"
                       "}\n\n"
                       ".card {\n"
                       "  max-width: 32rem;\n"
                       "  padding: 2rem;\n"
                       "  border: 1px solid #8884;\n"
                       "  border-radius: 0.75rem;\n"
                       "}"),
        QStringLiteral("document.querySelector(\"#hello\").addEventListener(\"click\", () => {\n"
                       "  console.log(\"Hello from ZealRN\");\n"
                       "});")};
}

QString previewRenderScript(const Documents &documents)
{
    const auto encoded = [](const QString &text) {
        return QString::fromLatin1(text.toUtf8().toBase64());
    };
    return QStringLiteral("window.zealrnPreview.render(\"%1\",\"%2\",\"%3\");")
        .arg(encoded(documents.html), encoded(documents.css), encoded(documents.javascript));
}

bool isBlockedPreviewUrl(const QUrl &url)
{
    static const QStringList blocked = {QStringLiteral("http"),
                                        QStringLiteral("https"),
                                        QStringLiteral("ftp"),
                                        QStringLiteral("ws"),
                                        QStringLiteral("wss"),
                                        QStringLiteral("file")};
    if (blocked.contains(url.scheme(), Qt::CaseInsensitive)) {
        return true;
    }
    return url.scheme().compare(QStringLiteral("qrc"), Qt::CaseInsensitive) == 0
           && url != QUrl(QStringLiteral("qrc:/playground/preview.html"));
}

void appendBounded(QStringList &messages, const QString &message, qsizetype maximum)
{
    if (maximum <= 0) {
        messages.clear();
        return;
    }
    while (messages.size() >= maximum) {
        messages.removeFirst();
    }
    messages.append(message);
}

QMap<QString, QByteArray> standaloneProject(const Documents &documents)
{
    const QString index = QStringLiteral("<!doctype html>\n"
                                         "<html>\n<head>\n"
                                         "  <meta charset=\"utf-8\">\n"
                                         "  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
                                         "  <link rel=\"stylesheet\" href=\"style.css\">\n"
                                         "</head>\n<body>\n%1\n"
                                         "  <script src=\"script.js\"></script>\n"
                                         "</body>\n</html>\n")
                              .arg(documents.html);
    return {{QStringLiteral("index.html"), index.toUtf8()},
            {QStringLiteral("script.js"), documents.javascript.toUtf8()},
            {QStringLiteral("style.css"), documents.css.toUtf8()}};
}

ExportResult writeProject(const QString &directory,
                          const Documents &documents,
                          bool overwrite,
                          QString *errorMessage)
{
    const auto files = standaloneProject(documents);
    const QDir dir(directory);
    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        if (!overwrite && QFileInfo::exists(dir.filePath(it.key()))) {
            return ExportResult::Exists;
        }
    }

    if (!QDir().mkpath(directory)) {
        if (errorMessage != nullptr) {
            *errorMessage = QObject::tr("Could not create project directory: %1").arg(directory);
        }
        return ExportResult::Error;
    }

    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        QSaveFile file(QDir(directory).filePath(it.key()));
        if (!file.open(QIODevice::WriteOnly) || file.write(it.value()) != it.value().size() || !file.commit()) {
            if (errorMessage != nullptr) {
                *errorMessage = QObject::tr("Could not write project file: %1").arg(file.fileName());
            }
            return ExportResult::Error;
        }
    }
    return ExportResult::Success;
}

} // namespace Zeal::WidgetUi::WebPlayground
