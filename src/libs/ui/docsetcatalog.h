// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_DOCSETCATALOG_H
#define ZEAL_WIDGETUI_DOCSETCATALOG_H

#include <registry/docsetmetadata.h>

#include <QByteArray>
#include <QList>
#include <QString>

#include <optional>

namespace Zeal::WidgetUi {

std::optional<QList<Registry::DocsetMetadata>> parseDocsetCatalog(const QByteArray &data, QString *error = nullptr);
bool writeDocsetCatalogCache(const QString &path, const QByteArray &data, QString *error = nullptr);

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_DOCSETCATALOG_H
