// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../settings.h"

#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using Appearance = Zeal::Core::Settings::ContentAppearance;
using Zeal::Core::Settings;

class SettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void streamRoundTrip_data();
    void streamRoundTrip();
    void invalidQsettingsValueFallsBackToSystem();
    void qsettingsRestoresAfterReopen_data();
    void qsettingsRestoresAfterReopen();
};

void SettingsTest::initTestCase()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QCoreApplication::setOrganizationName(QStringLiteral("ZealSettingsTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ZealSettingsTest"));
}

void SettingsTest::streamRoundTrip_data()
{
    QTest::addColumn<Appearance>("appearance");

    QTest::newRow("system") << Appearance::Automatic;
    QTest::newRow("light") << Appearance::Light;
    QTest::newRow("dark") << Appearance::Dark;
}

void SettingsTest::streamRoundTrip()
{
    QFETCH(Appearance, appearance);

    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out << appearance;

    Appearance restored = Appearance::Automatic;
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in >> restored;

    QCOMPARE(restored, appearance);
}

void SettingsTest::invalidQsettingsValueFallsBackToSystem()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        QSettings settings;
        settings.beginGroup(QStringLiteral("content"));
        settings.setValue(QStringLiteral("appearance"), QVariant::fromValue(static_cast<Appearance>(99)));
        settings.endGroup();
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
    }

    Settings settings;
    QCOMPARE(settings.contentAppearance, Appearance::Automatic);
}

void SettingsTest::qsettingsRestoresAfterReopen_data()
{
    streamRoundTrip_data();
}

void SettingsTest::qsettingsRestoresAfterReopen()
{
    QFETCH(Appearance, appearance);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("settings.ini"));

    {
        QSettings settings(path, QSettings::IniFormat);
        settings.setValue(QStringLiteral("content/appearance"), QVariant::fromValue(appearance));
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
    }

    QSettings settings(path, QSettings::IniFormat);
    const Appearance restored = settings.value(QStringLiteral("content/appearance")).value<Appearance>();
    QCOMPARE(restored, appearance);
}

QTEST_MAIN(SettingsTest)
#include "settings_test.moc"
