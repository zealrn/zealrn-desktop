// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../settings.h"
#include "../docsetstorage.h"

#include <QDir>
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
    void terminalSettingsRestoreAfterReopen();
    void terminalAutoStartDefaultsOn();
    void invalidTerminalFontSizeIsClamped();
    void invalidBottomToolFallsBackToWebPlayground();
    void noteEditorSettingsRestoreAfterReopen();
    void invalidNoteZoomIsClamped();
    void gettingStartedSettingsRestoreAfterReopen();
    void invalidGettingStartedValuesUseSafeDefaults();
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
