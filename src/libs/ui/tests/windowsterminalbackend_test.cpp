// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../terminalbackend.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <windows.h>

using namespace Zeal::WidgetUi;

namespace {
TerminalProfile cmdProfile(const QStringList &arguments = {})
{
    const QString command = QStandardPaths::findExecutable(QStringLiteral("cmd.exe"));
    return {command, QStringLiteral("Command Prompt"), command, arguments};
}

QString encodedPowerShellCommand(const QString &command)
{
    const QByteArray utf16(reinterpret_cast<const char *>(command.utf16()), command.size() * int(sizeof(char16_t)));
    return QString::fromLatin1(utf16.toBase64());
}
}

class WindowsTerminalBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void runsCmdWithWorkingDirectoryAndExitCode();
    void resizesAndInterruptsCmd();
    void preservesPowerShellUnicode();
    void destructionTerminatesProcessTree();
    void destructionWithPendingOutputDoesNotDeadlock();
};

void WindowsTerminalBackendTest::runsCmdWithWorkingDirectoryAndExitCode()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    auto backend = createTerminalBackend();
    QVERIFY(backend->isAvailable());
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QSignalSpy exited(backend.get(), &TerminalBackend::exited);

    QVERIFY(backend->start(cmdProfile({QStringLiteral("/D"), QStringLiteral("/Q")}),
                           directory.path(),
                           QSize(90, 30)));
    backend->write(QByteArrayLiteral("echo ZEALRN_CONPTY\r\ncd\r\nexit /b 7\r\n"));
    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 10000);

    QTRY_VERIFY_WITH_TIMEOUT(output.contains("ZEALRN_CONPTY"), 3000);
    QTRY_VERIFY_WITH_TIMEOUT(output.toLower().contains(QDir::toNativeSeparators(directory.path()).toUtf8().toLower()),
                             3000);
    QCOMPARE(exited.first().at(0).toInt(), 7);
    QVERIFY(exited.first().at(1).toBool());
}

void WindowsTerminalBackendTest::resizesAndInterruptsCmd()
{
    auto backend = createTerminalBackend();
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QSignalSpy exited(backend.get(), &TerminalBackend::exited);
    QSignalSpy errors(backend.get(), &TerminalBackend::errorOccurred);

    QVERIFY(backend->start(cmdProfile({QStringLiteral("/D"), QStringLiteral("/Q")}),
                           QDir::homePath(),
                           QSize(80, 24)));
    backend->resize(QSize(100, 35));
    QCOMPARE(errors.count(), 0);

    backend->write(QByteArrayLiteral("ping -t 127.0.0.1\r\n"));
    QTRY_VERIFY_WITH_TIMEOUT(output.toLower().contains("reply from"), 5000);
    backend->interrupt();
    QTest::qWait(250);
    backend->write(QByteArrayLiteral("echo INTERRUPTED\r\nexit /b 0\r\n"));

    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 10000);
    QVERIFY(output.contains("INTERRUPTED"));
}

void WindowsTerminalBackendTest::preservesPowerShellUnicode()
{
    const QString powershell = QStandardPaths::findExecutable(QStringLiteral("pwsh.exe"));
    if (powershell.isEmpty()) {
        QSKIP("PowerShell 7 is unavailable");
    }
    auto backend = createTerminalBackend();
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QSignalSpy exited(backend.get(), &TerminalBackend::exited);
    const QString expected = QString::fromUtf8("ZealRN \xe2\x82\xac \xe6\xbc\xa2\xe5\xad\x97");
    const TerminalProfile profile = {powershell,
                                     QStringLiteral("PowerShell 7"),
                                     powershell,
                                     {QStringLiteral("-NoLogo"),
                                      QStringLiteral("-NoProfile"),
                                      QStringLiteral("-EncodedCommand"),
                                      encodedPowerShellCommand(QStringLiteral("Write-Output '%1'").arg(expected))}};

    QVERIFY(backend->start(profile, QDir::homePath(), QSize(80, 24)));
    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 10000);
    QTRY_VERIFY_WITH_TIMEOUT(output.contains(expected.toUtf8()), 3000);
}

void WindowsTerminalBackendTest::destructionTerminatesProcessTree()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString pidPath = directory.filePath(QStringLiteral("processes.pid"));
    auto backend = createTerminalBackend();
    const QString powershell = QStandardPaths::findExecutable(QStringLiteral("powershell.exe"));
    QVERIFY(!powershell.isEmpty());
    const QString script = QStringLiteral(
                               "$child = Start-Process -FilePath $env:ComSpec "
                               "-ArgumentList '/D','/Q','/C','ping -t 127.0.0.1' -PassThru -WindowStyle Hidden; "
                               "@($PID, $child.Id) | Set-Content -Encoding ascii -LiteralPath '%1'; "
                               "while ($true) { Start-Sleep 1 }")
                               .arg(QDir::toNativeSeparators(pidPath).replace(QLatin1Char('\''), QStringLiteral("''")));
    const TerminalProfile profile = {powershell,
                                     QStringLiteral("Windows PowerShell"),
                                     powershell,
                                     {QStringLiteral("-NoLogo"),
                                      QStringLiteral("-NoProfile"),
                                      QStringLiteral("-EncodedCommand"),
                                      encodedPowerShellCommand(script)}};

    QVERIFY(backend->start(profile, directory.path(), QSize(80, 24)));
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pidPath), 5000);

    QFile pidFile(pidPath);
    QVERIFY(pidFile.open(QIODevice::ReadOnly));
    bool ok = false;
    QList<QByteArray> processIds;
    for (const QByteArray &line : pidFile.readAll().split('\n')) {
        if (!line.trimmed().isEmpty()) {
            processIds.append(line);
        }
    }
    QCOMPARE(processIds.size(), 2);
    QList<HANDLE> processes;
    for (const QByteArray &line : processIds) {
        const DWORD processId = line.trimmed().toULong(&ok);
        QVERIFY(ok);
        HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, processId);
        QVERIFY(process != nullptr);
        processes.append(process);
    }
    backend.reset();

    for (HANDLE process : processes) {
        QCOMPARE(WaitForSingleObject(process, 5000), DWORD(WAIT_OBJECT_0));
        CloseHandle(process);
    }
}

void WindowsTerminalBackendTest::destructionWithPendingOutputDoesNotDeadlock()
{
    auto backend = createTerminalBackend();
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QVERIFY(backend->start(cmdProfile({QStringLiteral("/D"), QStringLiteral("/Q")}),
                           QDir::homePath(),
                           QSize(80, 24)));
    backend->write(QByteArrayLiteral("for /L %i in (1,1,10000) do @echo ZEALRN_PENDING_%i\r\n"));
    QTRY_VERIFY_WITH_TIMEOUT(output.contains("ZEALRN_PENDING_1"), 3000);

    QElapsedTimer timer;
    timer.start();
    backend.reset();
    QVERIFY2(timer.elapsed() < 5000, "ConPTY shutdown blocked while output was pending");
}

QTEST_GUILESS_MAIN(WindowsTerminalBackendTest)
#include "windowsterminalbackend_test.moc"
