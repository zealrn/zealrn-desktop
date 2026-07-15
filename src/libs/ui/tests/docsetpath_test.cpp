#include <registry/docset.h>

#include <QTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <sqlite3.h>

using Zeal::Registry::Docset;

class DocsetPathTest : public QObject
{
    Q_OBJECT

private slots:
    void acceptsRelativeDocumentPath();
    void rejectsMissingAndUnsafePathForms();
    void checksExtractedDocsetFiles();
};

void DocsetPathTest::acceptsRelativeDocumentPath()
{
    QVERIFY(Docset::isSafeDocumentPath(QStringLiteral("doc/index.html")));
    QVERIFY(Docset::isSafeDocumentPath(QStringLiteral("C++/operator*.html")));
}

void DocsetPathTest::checksExtractedDocsetFiles()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString root = temp.filePath(QStringLiteral("Sample.docset"));
    QVERIFY(QDir().mkpath(root + QStringLiteral("/Contents/Resources/Documents")));

    QFile plist(root + QStringLiteral("/Contents/Info.plist"));
    QVERIFY(plist.open(QIODevice::WriteOnly));
    plist.write("<?xml version=\"1.0\"?><plist><dict><key>CFBundleName</key><string>Sample</string></dict></plist>");
    plist.close();

    QFile page(root + QStringLiteral("/Contents/Resources/Documents/index.html"));
    QVERIFY(page.open(QIODevice::WriteOnly));
    page.write("<html></html>");
    page.close();
    QVERIFY(QDir().mkpath(root + QStringLiteral("/Contents/Resources/Documents/folder")));

    sqlite3 *database = nullptr;
    QVERIFY(sqlite3_open((root + QStringLiteral("/Contents/Resources/docSet.dsidx")).toUtf8().constData(), &database)
            == SQLITE_OK);
    QVERIFY(sqlite3_exec(database, "CREATE TABLE searchIndex(name TEXT, type TEXT, path TEXT);", nullptr, nullptr, nullptr)
            == SQLITE_OK);
    sqlite3_close(database);

    const Docset docset(root);
    QVERIFY(docset.isValid());
    QVERIFY(docset.hasDocument(QStringLiteral("index.html")));
    QVERIFY(!docset.hasDocument(QStringLiteral("missing.html")));
    QVERIFY(!docset.hasDocument(QStringLiteral("folder")));
    QVERIFY(!docset.hasDocument(QStringLiteral("../index.html")));
}

void DocsetPathTest::rejectsMissingAndUnsafePathForms()
{
    QVERIFY(!Docset::isSafeDocumentPath(QString()));
    QVERIFY(!Docset::isSafeDocumentPath(QStringLiteral(".")));
    QVERIFY(!Docset::isSafeDocumentPath(QStringLiteral("../index.html")));
    QVERIFY(!Docset::isSafeDocumentPath(QStringLiteral("a/../../index.html")));
    QVERIFY(!Docset::isSafeDocumentPath(QStringLiteral("/index.html")));
    QVERIFY(!Docset::isSafeDocumentPath(QStringLiteral("C:/index.html")));
}

QTEST_MAIN(DocsetPathTest)
#include "docsetpath_test.moc"
