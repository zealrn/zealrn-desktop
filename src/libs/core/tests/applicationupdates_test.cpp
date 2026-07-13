// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../application.h"

#include <QtTest>

using Zeal::Core::Application;

class ApplicationUpdatesTest : public QObject
{
    Q_OBJECT

private slots:
    void usesZealrnRepository();
    void emptyListMeansNoPublishedReleases();
    void ignoresDraftsAndPrereleases();
    void rejectsMalformedJson();
};

void ApplicationUpdatesTest::usesZealrnRepository()
{
    QCOMPARE(Application::releasesApiUrl().toString(),
             QStringLiteral("https://api.github.com/repos/abnzrdev/zealrn/releases"));
}

void ApplicationUpdatesTest::emptyListMeansNoPublishedReleases()
{
    QString error;
    const auto version = Application::latestPublishedRelease(QByteArrayLiteral("[]"), &error);

    QVERIFY(!version.has_value());
    QVERIFY(error.isEmpty());
}

void ApplicationUpdatesTest::ignoresDraftsAndPrereleases()
{
    const QByteArray json = QByteArrayLiteral(R"([
        {"tag_name":"v9.0.0","draft":true,"prerelease":false},
        {"tag_name":"v8.0.0-beta.1","draft":false,"prerelease":true},
        {"tag_name":"v0.2.0","draft":false,"prerelease":false},
        {"tag_name":"v0.1.5","draft":false,"prerelease":false}
    ])");
    QString error;
    const auto version = Application::latestPublishedRelease(json, &error);

    QVERIFY2(version.has_value(), qPrintable(error));
    QCOMPARE(version->toString(), QStringLiteral("0.2.0"));
}

void ApplicationUpdatesTest::rejectsMalformedJson()
{
    QString error;
    const auto version = Application::latestPublishedRelease(QByteArrayLiteral("not json"), &error);

    QVERIFY(!version.has_value());
    QVERIFY(!error.isEmpty());
}

QTEST_APPLESS_MAIN(ApplicationUpdatesTest)
#include "applicationupdates_test.moc"
