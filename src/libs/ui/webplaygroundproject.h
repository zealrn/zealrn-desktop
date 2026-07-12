// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDPROJECT_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDPROJECT_H

#include <QMap>
#include <QString>

class QUrl;

namespace Zeal::WidgetUi::WebPlayground {

struct Documents
{
    QString html;
    QString css;
    QString javascript;

    bool operator==(const Documents &) const = default;
};

enum class ExportResult {
    Success,
    Exists,
    Error
};

Documents starterDocuments();
QString previewRenderScript(const Documents &documents);
bool isBlockedPreviewUrl(const QUrl &url);
void appendBounded(QStringList &messages, const QString &message, qsizetype maximum = 500);
QMap<QString, QByteArray> standaloneProject(const Documents &documents);
ExportResult writeProject(const QString &directory,
                          const Documents &documents,
                          bool overwrite,
                          QString *errorMessage = nullptr);

} // namespace Zeal::WidgetUi::WebPlayground

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDPROJECT_H
