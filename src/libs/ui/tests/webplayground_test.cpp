// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../webplaygroundproject.h"

#include <QFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

using namespace Zeal::WidgetUi::WebPlayground;

class WebPlaygroundTest : public QObject
{
    Q_OBJECT

private slots:
    void starterAndResetContent();
    void previewCompositionEncodesUserText();
    void blockedSchemes_data();
    void blockedSchemes();
    void consoleLimit();
    void standaloneOutput();
    void exportRefusesOverwrite();
};

void WebPlaygroundTest::starterAndResetContent()
{
    const Documents starter = starterDocuments();
    QVERIFY(starter.html.contains(QStringLiteral("Hello ZealRN")));
    QVERIFY(starter.css.contains(QStringLiteral(".card")));
    QVERIFY(starter.javascript.contains(QStringLiteral("console.log")));
    QCOMPARE(starter, starterDocuments());
}

void WebPlaygroundTest::previewCompositionEncodesUserText()
{
    const Documents documents = {QString::fromUtf8("<p>Привет 🌍</p>"),
                                 QStringLiteral("x::after { content: \"</style>\"; }"),
                                 QStringLiteral("const value = `</script>`;\nconsole.log(value);")};
    const QString script = previewRenderScript(documents);
    QVERIFY(script.startsWith(QStringLiteral("window.zealrnPreview.render(")));
    QVERIFY(!script.contains(documents.html));
    QVERIFY(!script.contains(QStringLiteral("</style>")));
    QVERIFY(!script.contains(QStringLiteral("</script>")));
    QVERIFY(script.contains(QString::fromLatin1(documents.html.toUtf8().toBase64())));
}

void WebPlaygroundTest::blockedSchemes_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("blocked");
    QTest::newRow("http") << QStringLiteral("http://example.com") << true;
    QTest::newRow("https") << QStringLiteral("https://example.com") << true;
    QTest::newRow("ftp") << QStringLiteral("ftp://example.com/a") << true;
    QTest::newRow("websocket") << QStringLiteral("wss://example.com") << true;
    QTest::newRow("file") << QStringLiteral("file:///tmp/private") << true;
    QTest::newRow("resource") << QStringLiteral("qrc:/playground/preview.html") << false;
    QTest::newRow("data") << QStringLiteral("data:text/plain,ok") << false;
    QTest::newRow("blob") << QStringLiteral("blob:null/id") << false;
}

void WebPlaygroundTest::blockedSchemes()
{
    QFETCH(QString, url);
    QFETCH(bool, blocked);
    QCOMPARE(isBlockedPreviewUrl(QUrl(url)), blocked);
}

void WebPlaygroundTest::consoleLimit()
{
    QStringList messages;
    for (int i = 0; i < 505; ++i) {
        appendBounded(messages, QString::number(i));
    }
    QCOMPARE(messages.size(), 500);
    QCOMPARE(messages.first(), QStringLiteral("5"));
    QCOMPARE(messages.last(), QStringLiteral("504"));
}

void WebPlaygroundTest::standaloneOutput()
{
    const Documents documents = {QString::fromUtf8("<h1>Résumé</h1>"),
                                 QStringLiteral("h1 { color: red; }"),
                                 QStringLiteral("console.log('ok');")};
    const auto files = standaloneProject(documents);
    QCOMPARE(files.size(), 3);
    QVERIFY(files.value(QStringLiteral("index.html")).contains("style.css"));
    QVERIFY(files.value(QStringLiteral("index.html")).contains("script.js"));
    QVERIFY(files.value(QStringLiteral("index.html")).contains(documents.html.toUtf8()));
    QCOMPARE(files.value(QStringLiteral("style.css")), documents.css.toUtf8());
    QCOMPARE(files.value(QStringLiteral("script.js")), documents.javascript.toUtf8());
}

void WebPlaygroundTest::exportRefusesOverwrite()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile existing(dir.filePath(QStringLiteral("index.html")));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("keep");
    existing.close();

    QCOMPARE(writeProject(dir.path(), starterDocuments(), false), ExportResult::Exists);
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), QByteArray("keep"));
    existing.close();

    QCOMPARE(writeProject(dir.path(), starterDocuments(), true), ExportResult::Success);
    QVERIFY(QFileInfo::exists(dir.filePath(QStringLiteral("style.css"))));
    QVERIFY(QFileInfo::exists(dir.filePath(QStringLiteral("script.js"))));
}

QTEST_GUILESS_MAIN(WebPlaygroundTest)
#include "webplayground_test.moc"
