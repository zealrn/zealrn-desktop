// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../docsetcatalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QTemporaryDir>
#include <QTest>

using namespace Zeal::WidgetUi;

class DocsetCatalogTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesCurrentSchema();
    void rejectsMalformedAndEmptyCatalogs();
    void writesCacheAtomicallyAndCreatesParent();
    void liveCatalog_optIn();
};

static QByteArray currentCatalog()
{
    return QJsonDocument(QJsonArray{
                             QJsonObject{{QStringLiteral("name"), QStringLiteral("Python_3")},
                                         {QStringLiteral("title"), QStringLiteral("Python 3")},
                                         {QStringLiteral("sourceId"), QStringLiteral("com.kapeli")}},
                             QJsonObject{{QStringLiteral("name"), QStringLiteral("Git_Cheatsheet")},
                                         {QStringLiteral("title"), QStringLiteral("Git (cheatsheet)")},
                                         {QStringLiteral("sourceId"), QStringLiteral("com.kapeli")}},
                         })
        .toJson(QJsonDocument::Compact);
}

void DocsetCatalogTest::parsesCurrentSchema()
{
    QString error;
    const auto catalog = parseDocsetCatalog(currentCatalog(), &error);
    QVERIFY2(catalog.has_value(), qPrintable(error));
    QCOMPARE(catalog->size(), 2);
    QCOMPARE(catalog->first().name(), QStringLiteral("Python_3"));
}

void DocsetCatalogTest::rejectsMalformedAndEmptyCatalogs()
{
    QString error;
    QVERIFY(!parseDocsetCatalog(QByteArrayLiteral("{"), &error).has_value());
    QVERIFY(!error.isEmpty());
    QVERIFY(!parseDocsetCatalog(QByteArrayLiteral("[]"), &error).has_value());
    QVERIFY(!parseDocsetCatalog(QByteArrayLiteral("[{}]"), &error).has_value());
    QVERIFY(!parseDocsetCatalog(QByteArrayLiteral("[{\"name\":\"Python_3\",\"sourceId\":\"com.kapeli\"},{}]"),
                                &error)
                 .has_value());
}

void DocsetCatalogTest::writesCacheAtomicallyAndCreatesParent()
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("nested/com.kapeli.json"));
    QString error;
    QVERIFY2(writeDocsetCatalogCache(path, currentCatalog(), &error), qPrintable(error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), currentCatalog());
}

void DocsetCatalogTest::liveCatalog_optIn()
{
    if (!qEnvironmentVariableIsSet("ZEALRN_LIVE_CATALOG_TEST")) {
        QSKIP("Set ZEALRN_LIVE_CATALOG_TEST=1 to query the official catalog.");
    }

    qInfo("SSL support=%d build=%s runtime=%s systemCAs=%lld",
          QSslSocket::supportsSsl(),
          qPrintable(QSslSocket::sslLibraryBuildVersionString()),
          qPrintable(QSslSocket::sslLibraryVersionString()),
          static_cast<long long>(QSslConfiguration::systemCaCertificates().size()));

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(QStringLiteral("https://api.zealdocs.org/v1/docsets")));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = manager.get(request);
    QSignalSpy finished(reply, &QNetworkReply::finished);
    QVERIFY2(finished.wait(30000), "Catalog request timed out.");
    QVERIFY2(reply->error() == QNetworkReply::NoError, qPrintable(reply->errorString()));

    const QByteArray data = reply->readAll();
    QString error;
    const auto catalog = parseDocsetCatalog(data, &error);
    QVERIFY2(catalog.has_value(), qPrintable(error));
    QVERIFY(catalog->size() > 900);
    qInfo("Catalog status=%d bytes=%lld entries=%lld finalUrl=%s",
          reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
          static_cast<long long>(data.size()),
          static_cast<long long>(catalog->size()),
          qPrintable(reply->url().toString()));
    reply->deleteLater();
}

QTEST_MAIN(DocsetCatalogTest)
#include "docsetcatalog_test.moc"
