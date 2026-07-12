// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../terminalsupport.h"
#include "../terminalbackend.h"

#include <QTest>
#include <QTemporaryDir>

using namespace Zeal::WidgetUi::TerminalSupport;

class DeveloperTerminalTest : public QObject
{
    Q_OBJECT

private slots:
    void validatedShell_keepsAvailableChoice();
    void validatedShell_fallsBackToFirstAvailable();
    void validatedWorkingDirectory_usesSafeFallbackOrder();
    void bottomToolFromValue_rejectsInvalidValue();
    void linuxExternalTerminal_buildsSeparateArguments();
    void windowsExternalTerminal_prefersWindowsTerminal();
    void windowsExternalTerminal_withoutWtUsesSelectedShell();
    void backendFactory_matchesBuildConfiguration();
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
#ifdef ZEALRN_HAVE_QTERMWIDGET
    QVERIFY(backend->isAvailable());
    QVERIFY(backend->widget());
#else
    QVERIFY(!backend->isAvailable());
    QVERIFY(!backend->widget());
#endif
}

QTEST_MAIN(DeveloperTerminalTest)
#include "developerterminal_test.moc"
