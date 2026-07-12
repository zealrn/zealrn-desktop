// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_DOCSETSTORAGE_H
#define ZEAL_CORE_DOCSETSTORAGE_H

#include <QStringList>

namespace Zeal::Core {

QStringList existingZealDocsetPaths(const QString &homePath);

} // namespace Zeal::Core

#endif // ZEAL_CORE_DOCSETSTORAGE_H
