// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTEPAGE_H
#define ZEAL_WIDGETUI_LEARNINGNOTEPAGE_H

#include <QDateTime>
#include <QString>
#include <QUrl>

namespace Zeal::WidgetUi {

struct LearningNotePage
{
    QString docsetId;
    QString docsetName;
    QString pageKey;
    QString pagePath;
    QString pageUrl;
    QString pageTitle;

    bool isValid() const { return !docsetId.isEmpty() && !pageKey.isEmpty(); }
    bool isStartNote() const;

    static LearningNotePage startNote();

    static LearningNotePage fromUrl(const QString &docsetId,
                                    const QString &docsetName,
                                    const QUrl &baseUrl,
                                    const QUrl &url,
                                    const QString &pageTitle);
};

struct LearningNote
{
    qint64 id = 0;
    LearningNotePage page;
    QString content;
    QDateTime createdAt;
    QDateTime updatedAt;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_LEARNINGNOTEPAGE_H
