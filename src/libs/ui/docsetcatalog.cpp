// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docsetcatalog.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>

namespace Zeal::WidgetUi {

std::optional<QList<Registry::DocsetMetadata>> parseDocsetCatalog(const QByteArray &data, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isArray()) {
        if (error != nullptr) {
            *error = parseError.error == QJsonParseError::NoError ? QObject::tr("Catalog is not a JSON array.")
                                                                 : parseError.errorString();
        }
        return std::nullopt;
    }

    QList<Registry::DocsetMetadata> catalog;
    for (const QJsonValue &value : document.array()) {
        const Registry::DocsetMetadata metadata(value.toObject());
        if (metadata.name().isEmpty() || metadata.urls().isEmpty()) {
            if (error != nullptr) {
                *error = QObject::tr("Catalog contains an invalid docset entry.");
            }
            return std::nullopt;
        }
        catalog.append(metadata);
    }

    if (catalog.isEmpty()) {
        if (error != nullptr) {
            *error = QObject::tr("Catalog contains no valid docsets.");
        }
        return std::nullopt;
    }

    if (error != nullptr) {
        error->clear();
    }
    return catalog;
}

bool writeDocsetCatalogCache(const QString &path, const QByteArray &data, QString *error)
{
    const QString parent = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(parent)) {
        if (error != nullptr) {
            *error = QObject::tr("Cannot create cache directory %1.").arg(parent);
        }
        return false;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        if (error != nullptr) {
            *error = file.errorString();
        }
        return false;
    }

    if (error != nullptr) {
        error->clear();
    }
    return true;
}

} // namespace Zeal::WidgetUi
