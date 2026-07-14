// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webbridge.h"

#include <core/application.h>
#include <core/settings.h>

#include <QDesktopServices>
#include <QSet>
#include <QUrl>

namespace Zeal::Browser {

namespace {
const QSet<QString> AllowedShortUrlKeys = {QStringLiteral("discord"),
                                           QStringLiteral("github"),
                                           QStringLiteral("report-bug"),
                                           QStringLiteral("telegram"),
                                           QStringLiteral("website"),
                                           QStringLiteral("x")};
} // namespace

WebBridge::WebBridge(QObject *parent)
    : QObject(parent)
{
    if (Core::Application::instance() != nullptr) {
        connect(Core::Application::instance()->settings(), &Core::Settings::updated, this, &WebBridge::gettingStartedChanged);
    }
}

void WebBridge::openShortUrl(const QString &key)
{
    if (!AllowedShortUrlKeys.contains(key)) {
        return;
    }

    if (key == QLatin1String("github")) {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn")));
    } else if (key == QLatin1String("report-bug")) {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn/issues/new")));
    } else {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://go.zealdocs.org/l/") + key));
    }
}

void WebBridge::triggerAction(const QString &action)
{
    emit actionTriggered(action);
}

void WebBridge::dismissGettingStartedChecklist()
{
    Core::Settings *settings = Core::Application::instance()->settings();
    settings->gettingStartedChecklistDismissed = true;
    settings->save();
}

quint32 WebBridge::gettingStartedChecklist() const
{
    return Core::Application::instance()->settings()->gettingStartedChecklist;
}

bool WebBridge::gettingStartedChecklistDismissed() const
{
    return Core::Application::instance()->settings()->gettingStartedChecklistDismissed;
}

QString WebBridge::appVersion()
{
    return Core::Application::versionString();
}

} // namespace Zeal::Browser
