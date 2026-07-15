// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../application.h"

#include <QtTest>

using Zeal::Core::Application;

class ApplicationUpdatesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void currentVersionIsProductionRelease();
    void usesZealrnRepository();
    void usesLatestStableReleaseEndpoint();
    void usesReleaseListForPrereleases();
    void usesZealrnReleasePage();
    void emptyListMeansNoPublishedReleases();
    void ignoresDraftsAndPrereleases();
    void parsesPublishedReleaseMetadata();
    void includesPrereleasesWhenRequested();
    void comparesSemanticVersions_data();
    void comparesSemanticVersions();
    void rejectsUntrustedReleaseUrl();
    void rejectsMalformedJson();
};

void ApplicationUpdatesTest::initTestCase()
{
    QCoreApplication::setApplicationVersion(QStringLiteral(ZEAL_VERSION));
}

void ApplicationUpdatesTest::currentVersionIsProductionRelease()
{
    QCOMPARE(Application::versionString(), QStringLiteral("v0.1.0"));
}

void ApplicationUpdatesTest::usesZealrnRepository()
{
    QCOMPARE(Application::repositorySlug(), QStringLiteral("abnzrdev/zealrn"));
}

void ApplicationUpdatesTest::usesLatestStableReleaseEndpoint()
{
    QCOMPARE(Application::releasesApiUrl(false).toString(),
             QStringLiteral("https://api.github.com/repos/abnzrdev/zealrn/releases/latest"));
}

void ApplicationUpdatesTest::usesReleaseListForPrereleases()
{
    QCOMPARE(Application::releasesApiUrl(true).toString(),
             QStringLiteral("https://api.github.com/repos/abnzrdev/zealrn/releases?per_page=20"));
}

void ApplicationUpdatesTest::usesZealrnReleasePage()
{
    QCOMPARE(Application::releasesPageUrl().toString(),
             QStringLiteral("https://github.com/abnzrdev/zealrn/releases"));
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
        {"tag_name":"v9.0.0","draft":true,"prerelease":false,"published_at":"2026-01-01T00:00:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v9.0.0"},
        {"tag_name":"v8.0.0-beta.1","draft":false,"prerelease":true,"published_at":"2026-01-01T00:00:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v8.0.0-beta.1"},
        {"tag_name":"v0.2.0","name":"ZealRN 0.2.0","draft":false,"prerelease":false,"published_at":"2026-01-01T00:00:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v0.2.0"},
        {"tag_name":"v0.1.5","draft":false,"prerelease":false,"published_at":"2025-01-01T00:00:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v0.1.5"}
    ])");
    QString error;
    const auto version = Application::latestPublishedRelease(json, &error);

    QVERIFY2(version.has_value(), qPrintable(error));
    QCOMPARE(version->toString(), QStringLiteral("0.2.0"));
}

void ApplicationUpdatesTest::parsesPublishedReleaseMetadata()
{
    const QByteArray json = QByteArrayLiteral(R"({
        "tag_name":"v0.2.0",
        "name":"Security update",
        "draft":false,
        "prerelease":false,
        "published_at":"2026-07-15T10:30:00Z",
        "html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v0.2.0"
    })");
    QString error;
    const auto release = Application::publishedRelease(json, false, &error);

    QVERIFY2(release.has_value(), qPrintable(error));
    QCOMPARE(release->version, QStringLiteral("0.2.0"));
    QCOMPARE(release->title, QStringLiteral("Security update"));
    QCOMPARE(release->publishedAt, QDateTime::fromString(QStringLiteral("2026-07-15T10:30:00Z"), Qt::ISODate));
    QCOMPARE(release->pageUrl,
             QUrl(QStringLiteral("https://github.com/abnzrdev/zealrn/releases/tag/v0.2.0")));
    QVERIFY(!release->prerelease);
}

void ApplicationUpdatesTest::includesPrereleasesWhenRequested()
{
    const QByteArray json = QByteArrayLiteral(R"([
        {"tag_name":"v0.2.0-beta.2","name":"Beta 2","draft":false,"prerelease":true,"published_at":"2026-07-15T10:30:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v0.2.0-beta.2"},
        {"tag_name":"v0.2.0-beta.1","name":"Beta 1","draft":false,"prerelease":true,"published_at":"2026-07-14T10:30:00Z","html_url":"https://github.com/abnzrdev/zealrn/releases/tag/v0.2.0-beta.1"}
    ])");
    QString error;
    const auto release = Application::publishedRelease(json, true, &error);

    QVERIFY2(release.has_value(), qPrintable(error));
    QCOMPARE(release->version, QStringLiteral("0.2.0-beta.2"));
    QVERIFY(release->prerelease);
}

void ApplicationUpdatesTest::comparesSemanticVersions_data()
{
    QTest::addColumn<QString>("candidate");
    QTest::addColumn<QString>("installed");
    QTest::addColumn<int>("comparison");

    QTest::newRow("new patch") << QStringLiteral("0.1.1") << QStringLiteral("0.1.0") << 1;
    QTest::newRow("same") << QStringLiteral("0.1.0") << QStringLiteral("0.1.0") << 0;
    QTest::newRow("older") << QStringLiteral("0.0.9") << QStringLiteral("0.1.0") << -1;
    QTest::newRow("stable after rc") << QStringLiteral("1.0.0") << QStringLiteral("1.0.0-rc.1") << 1;
    QTest::newRow("rc ordering") << QStringLiteral("1.0.0-rc.10") << QStringLiteral("1.0.0-rc.2") << 1;
    QTest::newRow("numeric before text") << QStringLiteral("1.0.0-1") << QStringLiteral("1.0.0-alpha") << -1;
}

void ApplicationUpdatesTest::comparesSemanticVersions()
{
    QFETCH(QString, candidate);
    QFETCH(QString, installed);
    QFETCH(int, comparison);
    QCOMPARE(qBound(-1, Application::compareSemanticVersions(candidate, installed), 1), comparison);
}

void ApplicationUpdatesTest::rejectsUntrustedReleaseUrl()
{
    const QByteArray json = QByteArrayLiteral(R"({
        "tag_name":"v9.0.0","name":"Fake","draft":false,"prerelease":false,
        "published_at":"2026-07-15T10:30:00Z","html_url":"https://example.com/zealrn.exe"
    })");
    QString error;
    const auto release = Application::publishedRelease(json, false, &error);

    QVERIFY(!release.has_value());
    QVERIFY(!error.isEmpty());
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
