// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docsetstorage.h"

#include <QDir>

namespace Zeal::Core {

QStringList existingZealDocsetPaths(const QString &homePath)
{
    const QDir home(homePath);
    const QStringList candidates = {
        home.filePath(QStringLiteral(".var/app/org.zealdocs.Zeal/data/Zeal/Zeal/docsets")),
        home.filePath(QStringLiteral(".local/share/Zeal/Zeal/docsets"))};
    QStringList existing;
    for (const QString &candidate : candidates) {
        if (QDir(candidate).exists()) {
            existing.append(candidate);
        }
    }
    return existing;
}

} // namespace Zeal::Core
