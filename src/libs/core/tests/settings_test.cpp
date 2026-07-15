// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../settings.h"
#include "../docsetstorage.h"

#include <QDir>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using Appearance = Zeal::Core::Settings::ContentAppearance;
using UpdateFrequency = Zeal::Core::Settings::UpdateFrequency;
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
    void terminalSettingsRestoreAfterReopen();
    void terminalAutoStartDefaultsOn();
    void invalidTerminalFontSizeIsClamped();
    void invalidBottomToolFallsBackToWebPlayground();
    void noteEditorSettingsRestoreAfterReopen();
    void invalidNoteZoomIsClamped();
    void gettingStartedSettingsRestoreAfterReopen();
    void invalidGettingStartedValuesUseSafeDefaults();
    void updateSettingsRestoreAfterReopen();
    void invalidUpdateFrequencyFallsBackToDaily();
    void automaticUpdateDue_data();
    void automaticUpdateDue();
    void sharedDocsetSettingRestoresAfterReopen();
    void existingZealDocsetPathsFindsNativeAndFlatpakLibraries();
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

void SettingsTest::terminalSettingsRestoreAfterReopen()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        Settings settings;
        settings.terminalSafetyAcknowledged = true;
        settings.terminalShell = QStringLiteral("/bin/zsh");
        settings.terminalWorkingDirectory = QStringLiteral("/home/test/work");
        settings.terminalFontSize = 18;
        settings.terminalStartOnOpen = false;
        settings.bottomDevelopmentTool = 1;
        settings.save();
    }

    Settings restored;
    QVERIFY(restored.terminalSafetyAcknowledged);
    QCOMPARE(restored.terminalShell, QStringLiteral("/bin/zsh"));
    QCOMPARE(restored.terminalWorkingDirectory, QStringLiteral("/home/test/work"));
    QCOMPARE(restored.terminalFontSize, 18);
    QVERIFY(!restored.terminalStartOnOpen);
    QCOMPARE(restored.bottomDevelopmentTool, 1);
}

void SettingsTest::terminalAutoStartDefaultsOn()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    Settings settings;
    QVERIFY(settings.terminalStartOnOpen);
}

void SettingsTest::invalidTerminalFontSizeIsClamped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        QSettings settings;
        settings.setValue(QStringLiteral("ui/terminal_font_size"), 200);
        settings.sync();
    }

    Settings settings;
    QCOMPARE(settings.terminalFontSize, 28);
}

void SettingsTest::invalidBottomToolFallsBackToWebPlayground()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        QSettings settings;
        settings.setValue(QStringLiteral("ui/bottom_development_tool"), 99);
        settings.sync();
    }

    Settings settings;
    QCOMPARE(settings.bottomDevelopmentTool, 0);
}

void SettingsTest::noteEditorSettingsRestoreAfterReopen()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    {
        Settings settings;
        settings.learningNotesZoom = 145;
        settings.learningNotesLineWrap = false;
        settings.save();
    }

    Settings restored;
    QCOMPARE(restored.learningNotesZoom, 145);
    QVERIFY(!restored.learningNotesLineWrap);
}

void SettingsTest::invalidNoteZoomIsClamped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());

    QSettings settings;
    settings.setValue(QStringLiteral("ui/learning_notes_zoom"), 500);
    settings.sync();

    Settings restored;
    QCOMPARE(restored.learningNotesZoom, 200);
}

void SettingsTest::gettingStartedSettingsRestoreAfterReopen()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    {
        Settings settings;
        settings.quickTourShown = true;
        settings.quickTourCompleted = true;
        settings.quickTourNextLaunch = true;
        settings.openStartNoteOnLaunch = false;
        settings.openLastDocumentationOnLaunch = false;
        settings.lastDocumentationDocsetId = QStringLiteral("Python_3");
        settings.lastDocumentationPagePath = QStringLiteral("library/pathlib.html");
        settings.gettingStartedChecklist = 0x25;
        settings.gettingStartedChecklistDismissed = true;
        settings.dismissedHelpTips = {QStringLiteral("notes"), QStringLiteral("terminal")};
        settings.save();
    }

    Settings restored;
    QVERIFY(restored.quickTourShown);
    QVERIFY(restored.quickTourCompleted);
    QVERIFY(restored.quickTourNextLaunch);
    QVERIFY(!restored.openStartNoteOnLaunch);
    QVERIFY(!restored.openLastDocumentationOnLaunch);
    QCOMPARE(restored.lastDocumentationDocsetId, QStringLiteral("Python_3"));
    QCOMPARE(restored.lastDocumentationPagePath, QStringLiteral("library/pathlib.html"));
    QCOMPARE(restored.gettingStartedChecklist, quint32(0x25));
    QVERIFY(restored.gettingStartedChecklistDismissed);
    QCOMPARE(restored.dismissedHelpTips, QStringList({QStringLiteral("notes"), QStringLiteral("terminal")}));
}

void SettingsTest::invalidGettingStartedValuesUseSafeDefaults()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    QSettings raw;
    raw.setValue(QStringLiteral("getting_started/open_start_note"), QStringLiteral("invalid"));
    raw.setValue(QStringLiteral("getting_started/checklist"), -1);
    raw.sync();

    Settings restored;
    QVERIFY(restored.openStartNoteOnLaunch);
    QCOMPARE(restored.gettingStartedChecklist, quint32(0));
}

void SettingsTest::updateSettingsRestoreAfterReopen()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    const QDateTime attempted(QDate(2026, 7, 14), QTime(10, 0), QTimeZone::utc());
    const QDateTime successful(QDate(2026, 7, 14), QTime(10, 1), QTimeZone::utc());
    {
        Settings settings;
        settings.checkForUpdate = true;
        settings.updateFrequency = UpdateFrequency::Weekly;
        settings.updateIncludePrereleases = true;
        settings.updateLastAttempt = attempted;
        settings.updateLastSuccess = successful;
        settings.updateEtag = QByteArrayLiteral("\"release-etag\"");
        settings.updateSkippedVersion = QStringLiteral("0.2.0");
        settings.updateCachedVersion = QStringLiteral("0.2.1");
        settings.updateCachedTitle = QStringLiteral("ZealRN 0.2.1");
        settings.updateCachedPublishedAt = successful;
        settings.updateCachedPageUrl = QStringLiteral("https://github.com/abnzrdev/zealrn/releases/tag/v0.2.1");
        settings.updateCachedPrerelease = true;
        settings.save();
    }

    Settings restored;
    QVERIFY(restored.checkForUpdate);
    QCOMPARE(restored.updateFrequency, UpdateFrequency::Weekly);
    QVERIFY(restored.updateIncludePrereleases);
    QCOMPARE(restored.updateLastAttempt, attempted);
    QCOMPARE(restored.updateLastSuccess, successful);
    QCOMPARE(restored.updateEtag, QByteArrayLiteral("\"release-etag\""));
    QCOMPARE(restored.updateSkippedVersion, QStringLiteral("0.2.0"));
    QCOMPARE(restored.updateCachedVersion, QStringLiteral("0.2.1"));
    QCOMPARE(restored.updateCachedTitle, QStringLiteral("ZealRN 0.2.1"));
    QCOMPARE(restored.updateCachedPublishedAt, successful);
    QCOMPARE(restored.updateCachedPageUrl,
             QStringLiteral("https://github.com/abnzrdev/zealrn/releases/tag/v0.2.1"));
    QVERIFY(restored.updateCachedPrerelease);
}

void SettingsTest::invalidUpdateFrequencyFallsBackToDaily()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    QSettings raw;
    raw.setValue(QStringLiteral("updates/frequency"), 99);
    raw.sync();

    Settings settings;
    QCOMPARE(settings.updateFrequency, UpdateFrequency::Daily);
}

void SettingsTest::automaticUpdateDue_data()
{
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<UpdateFrequency>("frequency");
    QTest::addColumn<QDateTime>("lastAttempt");
    QTest::addColumn<bool>("due");

    const QDateTime now(QDate(2026, 7, 15), QTime(12, 0), QTimeZone::utc());
    QTest::newRow("disabled") << false << UpdateFrequency::Daily << QDateTime() << false;
    QTest::newRow("never") << true << UpdateFrequency::Never << QDateTime() << false;
    QTest::newRow("first check") << true << UpdateFrequency::Daily << QDateTime() << true;
    QTest::newRow("daily too soon") << true << UpdateFrequency::Daily << now.addSecs(-23 * 3600) << false;
    QTest::newRow("daily due") << true << UpdateFrequency::Daily << now.addSecs(-24 * 3600) << true;
    QTest::newRow("weekly too soon") << true << UpdateFrequency::Weekly << now.addDays(-6) << false;
    QTest::newRow("weekly due") << true << UpdateFrequency::Weekly << now.addDays(-7) << true;
    QTest::newRow("future clock") << true << UpdateFrequency::Daily << now.addDays(1) << true;
}

void SettingsTest::automaticUpdateDue()
{
    QFETCH(bool, enabled);
    QFETCH(UpdateFrequency, frequency);
    QFETCH(QDateTime, lastAttempt);
    QFETCH(bool, due);

    Settings settings;
    settings.checkForUpdate = enabled;
    settings.updateFrequency = frequency;
    settings.updateLastAttempt = lastAttempt;
    const QDateTime now(QDate(2026, 7, 15), QTime(12, 0), QTimeZone::utc());
    QCOMPARE(settings.isAutomaticUpdateCheckDue(now), due);
}

void SettingsTest::sharedDocsetSettingRestoresAfterReopen()
{
    QTemporaryDir dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
    {
        Settings settings;
        settings.docsetPathReadOnly = true;
        settings.save();
    }
    Settings restored;
    QVERIFY(restored.docsetPathReadOnly);
}

void SettingsTest::existingZealDocsetPathsFindsNativeAndFlatpakLibraries()
{
    QTemporaryDir home;
    const QString native = QDir(home.path()).filePath(QStringLiteral(".local/share/Zeal/Zeal/docsets"));
    const QString flatpak = QDir(home.path())
                                .filePath(QStringLiteral(".var/app/org.zealdocs.Zeal/data/Zeal/Zeal/docsets"));
    QVERIFY(QDir().mkpath(native));
    QVERIFY(QDir().mkpath(flatpak));

    QCOMPARE(Zeal::Core::existingZealDocsetPaths(home.path()), QStringList({flatpak, native}));
}

QTEST_MAIN(SettingsTest)
#include "settings_test.moc"
