// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_WEBBRIDGE_H
#define ZEAL_BROWSER_WEBBRIDGE_H

#include <QObject>

namespace Zeal::Browser {

class WebBridge final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebBridge)
    Q_PROPERTY(QString AppVersion READ appVersion CONSTANT)
    Q_PROPERTY(quint32 GettingStartedChecklist READ gettingStartedChecklist NOTIFY gettingStartedChanged)
    Q_PROPERTY(bool GettingStartedChecklistDismissed READ gettingStartedChecklistDismissed NOTIFY gettingStartedChanged)
public:
    explicit WebBridge(QObject *parent = nullptr);
    ~WebBridge() override = default;

    Q_INVOKABLE static void openShortUrl(const QString &key);
    Q_INVOKABLE void triggerAction(const QString &action);
    Q_INVOKABLE void dismissGettingStartedChecklist();

signals:
    void actionTriggered(const QString &action);
    void gettingStartedChanged();

private:
    static QString appVersion();
    quint32 gettingStartedChecklist() const;
    bool gettingStartedChecklistDismissed() const;
};

} // namespace Zeal::Browser

#endif // ZEAL_BROWSER_WEBBRIDGE_H
