// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <registry/docsetmetadata.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QTest>

using Zeal::Registry::DocsetMetadata;

class DocsetMetadataTest : public QObject
{
    Q_OBJECT

private slots:
    void currentCatalogSchema_providesDownloadUrl();
    void legacyCatalogSchema_keepsDownloadUrl();
};

void DocsetMetadataTest::currentCatalogSchema_providesDownloadUrl()
{
    const DocsetMetadata metadata(QJsonObject{{QStringLiteral("name"), QStringLiteral("Python_3")},
                                               {QStringLiteral("title"), QStringLiteral("Python 3")},
                                               {QStringLiteral("sourceId"), QStringLiteral("com.kapeli")},
                                               {QStringLiteral("versions"), QJsonArray{QStringLiteral("3.14.6")}}});

    QCOMPARE(metadata.url(), QUrl(QStringLiteral("https://go.zealdocs.org/d/com.kapeli/Python_3/latest")));
}

void DocsetMetadataTest::legacyCatalogSchema_keepsDownloadUrl()
{
    const QUrl url(QStringLiteral("https://example.invalid/Python.tgz"));
    const DocsetMetadata metadata(QJsonObject{{QStringLiteral("name"), QStringLiteral("Python_3")},
                                               {QStringLiteral("title"), QStringLiteral("Python 3")},
                                               {QStringLiteral("urls"), QJsonArray{url.toString()}}});

    QCOMPARE(metadata.url(), url);
}

QTEST_MAIN(DocsetMetadataTest)
#include "docsetmetadata_test.moc"
