// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../terminalsupport.h"
#include "../terminalbackend.h"
#include "../terminalbridge.h"
#include "../terminalview.h"
#include "../developerterminalpanel.h"

#include <core/settings.h>

#include <QSignalSpy>
#include <QSettings>
#include <QTest>
#include <QTemporaryDir>
#include <QUrl>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

using namespace Zeal::WidgetUi::TerminalSupport;

class DeveloperTerminalTest : public QObject
{
    Q_OBJECT

private slots:
    void validatedShell_keepsAvailableChoice();
    void validatedShell_fallsBackToFirstAvailable();
    void terminalProfiles_labelLinuxShells();
    void terminalProfiles_configureWindowsShells();
    void validatedTerminalProfile_fallsBackSafely();
    void terminalFontSize_isBounded();
    void windowsCommandLine_quotesArguments();
    void validatedWorkingDirectory_usesSafeFallbackOrder();
    void bottomToolFromValue_rejectsInvalidValue();
    void linuxExternalTerminal_buildsSeparateArguments();
    void windowsExternalTerminal_prefersWindowsTerminal();
    void windowsExternalTerminal_withoutWtUsesSelectedShell();
    void backendFactory_matchesBuildConfiguration();
    void terminalBridge_decodesInputBytes();
    void terminalBridge_queuesOutputUntilReady();
    void terminalBridge_batchesOutput();
    void terminalBridge_boundsPendingOutput();
    void terminalView_allowsOnlyLocalAssets();
    void terminalPanel_startsLazyWithSessionControls();
};

void DeveloperTerminalTest::validatedShell_keepsAvailableChoice()
{
    QCOMPARE(validatedShell(QStringLiteral("/bin/zsh"), {QStringLiteral("/bin/bash"), QStringLiteral("/bin/zsh")}),
             QStringLiteral("/bin/zsh"));
}

void DeveloperTerminalTest::validatedShell_fallsBackToFirstAvailable()
{
    QCOMPARE(validatedShell(QStringLiteral("/missing/shell"), {QStringLiteral("/bin/fish"), QStringLiteral("/bin/sh")}),
             QStringLiteral("/bin/fish"));
    QVERIFY(validatedShell(QStringLiteral("/missing/shell"), {}).isEmpty());
}

void DeveloperTerminalTest::terminalProfiles_labelLinuxShells()
{
    const auto profiles = terminalProfiles(Platform::Linux,
                                           {QStringLiteral("/bin/bash"),
                                            QStringLiteral("/usr/bin/zsh"),
                                            QStringLiteral("/usr/bin/fish"),
                                            QStringLiteral("/bin/custom-shell")});

    QCOMPARE(profiles.size(), 4);
    QCOMPARE(profiles.at(0).label, QStringLiteral("Bash"));
    QCOMPARE(profiles.at(1).label, QStringLiteral("Zsh"));
    QCOMPARE(profiles.at(2).label, QStringLiteral("Fish"));
    QCOMPARE(profiles.at(3).label, QStringLiteral("custom-shell"));
    QCOMPARE(profiles.at(0).program, QStringLiteral("/bin/bash"));
}

void DeveloperTerminalTest::terminalProfiles_configureWindowsShells()
{
    const auto profiles = terminalProfiles(Platform::Windows,
                                           {QStringLiteral("C:/Apps/pwsh.exe"),
                                            QStringLiteral("C:/Windows/System32/cmd.exe"),
                                            QStringLiteral("C:/Program Files/Git/bin/bash.exe")});

    QCOMPARE(profiles.at(0).label, QStringLiteral("PowerShell 7"));
    QCOMPARE(profiles.at(1).label, QStringLiteral("Command Prompt"));
    QCOMPARE(profiles.at(2).label, QStringLiteral("Git Bash"));
    QCOMPARE(profiles.at(2).arguments, QStringList({QStringLiteral("--login"), QStringLiteral("-i")}));
}

void DeveloperTerminalTest::validatedTerminalProfile_fallsBackSafely()
{
    const auto profiles = terminalProfiles(Platform::Linux,
                                           {QStringLiteral("/bin/bash"), QStringLiteral("/usr/bin/zsh")});

    QCOMPARE(validatedTerminalProfile(QStringLiteral("/usr/bin/zsh"), profiles).program,
             QStringLiteral("/usr/bin/zsh"));
    QCOMPARE(validatedTerminalProfile(QStringLiteral("/missing"), profiles).program, QStringLiteral("/bin/bash"));
    QVERIFY(validatedTerminalProfile(QStringLiteral("/missing"), {}).program.isEmpty());
}

void DeveloperTerminalTest::terminalFontSize_isBounded()
{
    QCOMPARE(clampTerminalFontSize(1), 10);
    QCOMPARE(clampTerminalFontSize(14), 14);
    QCOMPARE(clampTerminalFontSize(99), 28);
}

void DeveloperTerminalTest::windowsCommandLine_quotesArguments()
{
    const Zeal::WidgetUi::TerminalProfile profile = {QStringLiteral("pwsh"),
                                                     QStringLiteral("PowerShell"),
                                                     QStringLiteral("C:\\Program Files\\PowerShell\\pwsh.exe"),
                                                     {QStringLiteral("-NoLogo"),
                                                      QStringLiteral("two words"),
                                                      QStringLiteral("quote\"inside"),
                                                      QStringLiteral("C:\\trailing slash\\")}};

    QCOMPARE(windowsCommandLine(profile),
             QStringLiteral("\"C:\\Program Files\\PowerShell\\pwsh.exe\" -NoLogo \"two words\" "
                            "\"quote\\\"inside\" \"C:\\trailing slash\\\\\""));
}

void DeveloperTerminalTest::validatedWorkingDirectory_usesSafeFallbackOrder()
{
    QTemporaryDir home;
    QTemporaryDir workspace;
    QVERIFY(home.isValid());
    QVERIFY(workspace.isValid());

    QCOMPARE(validatedWorkingDirectory(QStringLiteral("/missing/path"), home.path(), workspace.path()), home.path());
    QCOMPARE(validatedWorkingDirectory(workspace.path(), home.path(), QStringLiteral("/missing/workspace")),
             workspace.path());
}

void DeveloperTerminalTest::bottomToolFromValue_rejectsInvalidValue()
{
    QCOMPARE(bottomToolFromValue(0), BottomTool::WebPlayground);
    QCOMPARE(bottomToolFromValue(1), BottomTool::DeveloperTerminal);
    QCOMPARE(bottomToolFromValue(-1), BottomTool::WebPlayground);
    QCOMPARE(bottomToolFromValue(42), BottomTool::WebPlayground);
}

void DeveloperTerminalTest::linuxExternalTerminal_buildsSeparateArguments()
{
    const ExternalTerminalLaunch launch = externalTerminalLaunch(Platform::Linux,
                                                                 {QStringLiteral("/usr/bin/konsole")},
                                                                 QStringLiteral("/bin/zsh"),
                                                                 QStringLiteral("/home/user/My Project"));

    QCOMPARE(launch.program, QStringLiteral("/usr/bin/konsole"));
    QCOMPARE(launch.arguments,
             QStringList({QStringLiteral("--workdir"),
                          QStringLiteral("/home/user/My Project"),
                          QStringLiteral("-e"),
                          QStringLiteral("/bin/zsh")}));
}

void DeveloperTerminalTest::windowsExternalTerminal_prefersWindowsTerminal()
{
    const ExternalTerminalLaunch launch = externalTerminalLaunch(Platform::Windows,
                                                                 {QStringLiteral("C:/Windows/System32/cmd.exe"),
                                                                  QStringLiteral("C:/Apps/wt.exe")},
                                                                 QStringLiteral("C:/Apps/pwsh.exe"),
                                                                 QStringLiteral("C:/Users/Test User"));

    QCOMPARE(launch.program, QStringLiteral("C:/Apps/wt.exe"));
    QCOMPARE(launch.arguments,
             QStringList({QStringLiteral("-d"),
                          QStringLiteral("C:/Users/Test User"),
                          QStringLiteral("C:/Apps/pwsh.exe")}));
}

void DeveloperTerminalTest::windowsExternalTerminal_withoutWtUsesSelectedShell()
{
    const ExternalTerminalLaunch launch = externalTerminalLaunch(Platform::Windows,
                                                                 {QStringLiteral("C:/Apps/pwsh.exe"),
                                                                  QStringLiteral("C:/Windows/System32/cmd.exe")},
                                                                 QStringLiteral("C:/Windows/System32/cmd.exe"),
                                                                 QStringLiteral("C:/Users/Test"));

    QCOMPARE(launch.program, QStringLiteral("C:/Windows/System32/cmd.exe"));
    QVERIFY(launch.arguments.isEmpty());
}

void DeveloperTerminalTest::backendFactory_matchesBuildConfiguration()
{
    const auto backend = Zeal::WidgetUi::createTerminalBackend();
    QVERIFY(backend);
#if defined(ZEALRN_HAVE_POSIX_PTY) || defined(ZEALRN_HAVE_CONPTY)
    QVERIFY(backend->isAvailable());
#else
    QVERIFY(!backend->isAvailable());
#endif
    QVERIFY(!backend->isRunning());
    QVERIFY(!backend->start({}, QStringLiteral("/missing"), QSize(80, 24)));
}

void DeveloperTerminalTest::terminalBridge_decodesInputBytes()
{
    Zeal::WidgetUi::TerminalBridge bridge;
    QSignalSpy spy(&bridge, &Zeal::WidgetUi::TerminalBridge::inputReceived);
    const QByteArray input("\x03\xe2\x82\xac", 4);

    bridge.sendInput(QString::fromLatin1(input.toBase64()));

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toByteArray(), input);
}

void DeveloperTerminalTest::terminalBridge_queuesOutputUntilReady()
{
    Zeal::WidgetUi::TerminalBridge bridge;
    QSignalSpy spy(&bridge, &Zeal::WidgetUi::TerminalBridge::outputReceived);
    QSignalSpy ready(&bridge, &Zeal::WidgetUi::TerminalBridge::ready);
    const QByteArray first("\xe2", 1);
    const QByteArray second("\x82\xac", 2);

    bridge.enqueueOutput(first);
    bridge.enqueueOutput(second);
    QCoreApplication::processEvents();
    QCOMPARE(spy.count(), 0);

    bridge.frontendReady();
    QCOMPARE(ready.count(), 1);
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(QByteArray::fromBase64(spy.takeFirst().at(0).toString().toLatin1()), first + second);
}

void DeveloperTerminalTest::terminalBridge_batchesOutput()
{
    Zeal::WidgetUi::TerminalBridge bridge;
    bridge.frontendReady();
    QSignalSpy spy(&bridge, &Zeal::WidgetUi::TerminalBridge::outputReceived);

    bridge.enqueueOutput(QByteArrayLiteral("one"));
    bridge.enqueueOutput(QByteArrayLiteral("two"));

    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(QByteArray::fromBase64(spy.takeFirst().at(0).toString().toLatin1()), QByteArrayLiteral("onetwo"));
}

void DeveloperTerminalTest::terminalBridge_boundsPendingOutput()
{
    Zeal::WidgetUi::TerminalBridge bridge;
    QSignalSpy errors(&bridge, &Zeal::WidgetUi::TerminalBridge::errorOccurred);
    QSignalSpy output(&bridge, &Zeal::WidgetUi::TerminalBridge::outputReceived);
    const QByteArray oversized(Zeal::WidgetUi::TerminalBridge::MaxPendingBytes + 32, 'x');

    bridge.enqueueOutput(oversized);
    bridge.frontendReady();

    QTRY_COMPARE(errors.count(), 1);
    QTRY_COMPARE(output.count(), 1);
    QCOMPARE(QByteArray::fromBase64(output.takeFirst().at(0).toString().toLatin1()).size(),
             Zeal::WidgetUi::TerminalBridge::MaxPendingBytes);
}

void DeveloperTerminalTest::terminalView_allowsOnlyLocalAssets()
{
    using Zeal::WidgetUi::TerminalView;

    QVERIFY(TerminalView::isAllowedUrl(QUrl(QStringLiteral("qrc:/terminal/index.html"))));
    QVERIFY(TerminalView::isAllowedUrl(QUrl(QStringLiteral("qrc:/terminal/terminal.bundle.js"))));
    QVERIFY(TerminalView::isAllowedUrl(QUrl(QStringLiteral("qrc:/qtwebchannel/qwebchannel.js"))));
    QVERIFY(!TerminalView::isAllowedUrl(QUrl(QStringLiteral("qrc:/playground/editor.html"))));
    QVERIFY(!TerminalView::isAllowedUrl(QUrl(QStringLiteral("https://example.com"))));
    QVERIFY(!TerminalView::isAllowedUrl(QUrl::fromLocalFile(QStringLiteral("/tmp/file"))));
    QVERIFY(!TerminalView::isAllowedUrl(QUrl(QStringLiteral("data:text/html,terminal"))));
}

void DeveloperTerminalTest::terminalPanel_startsLazyWithSessionControls()
{
    QTemporaryDir settingsDirectory;
    QVERIFY(settingsDirectory.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    QCoreApplication::setOrganizationName(QStringLiteral("ZealTerminalTest"));
    QCoreApplication::setApplicationName(QStringLiteral("ZealTerminalTest"));
    Zeal::Core::Settings settings;

    Zeal::WidgetUi::DeveloperTerminalPanel panel(&settings);

    QVERIFY(!panel.findChild<QWidget *>(QStringLiteral("terminalView")));
    QVERIFY(panel.findChild<QComboBox *>(QStringLiteral("terminalShell")));
    QVERIFY(panel.findChild<QPushButton *>(QStringLiteral("terminalStop")));
    QVERIFY(panel.findChild<QPushButton *>(QStringLiteral("terminalSearch")));
    const auto *fontSize = panel.findChild<QLabel *>(QStringLiteral("terminalFontSize"));
    QVERIFY(fontSize);
    QVERIFY(fontSize->text().contains(QString::number(settings.terminalFontSize)));
}

QTEST_MAIN(DeveloperTerminalTest)
#include "developerterminal_test.moc"
