// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../terminalbackend.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <cerrno>
#include <csignal>
#include <memory>
#include <unistd.h>

using namespace Zeal::WidgetUi;

namespace {
TerminalProfile shellProfile(const QString &command)
{
    return {QStringLiteral("sh"),
            QStringLiteral("sh"),
            QStringLiteral("/bin/sh"),
            {QStringLiteral("-c"), command}};
}
}

class PosixPtyBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void runsCommandWithTerminalEnvironment();
    void resizesTerminal();
    void interruptsForegroundCommand();
    void destructionReapsChild();
};

void PosixPtyBackendTest::runsCommandWithTerminalEnvironment()
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
    const QString command = QStringLiteral(
        "printf 'cwd:%s\\n' \"$PWD\"; printf '\\033[31mred\\033[0m\\n'; "
        "printf 'unicode:\\342\\202\\254\\n'; printf 'term:%s\\n' \"$TERM\"; stty size; exit 7");

    QVERIFY(backend->start(shellProfile(command), directory.path(), QSize(91, 31)));
    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 5000);

    const QByteArray expectedDirectory = QByteArrayLiteral("cwd:") + directory.path().toUtf8();
    const QByteArray expectedUnicode = QByteArrayLiteral("unicode:") + QByteArray::fromHex("e282ac");
    QVERIFY(output.contains(expectedDirectory));
    QVERIFY(output.contains("\x1b[31mred\x1b[0m"));
    QVERIFY(output.contains(expectedUnicode));
    QVERIFY(output.contains("term:xterm-256color"));
    QVERIFY(output.contains("31 91"));
    QCOMPARE(exited.first().at(0).toInt(), 7);
    QVERIFY(exited.first().at(1).toBool());
}

void PosixPtyBackendTest::resizesTerminal()
{
    auto backend = createTerminalBackend();
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QSignalSpy exited(backend.get(), &TerminalBackend::exited);

    QVERIFY(backend->start(shellProfile(QStringLiteral(
                               "trap 'stty size; exit 0' WINCH; printf ready; while :; do sleep 1; done")),
                           QDir::homePath(),
                           QSize(80, 24)));
    QTRY_VERIFY_WITH_TIMEOUT(output.contains("ready"), 3000);
    backend->resize(QSize(120, 40));

    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 5000);
    QVERIFY(output.contains("40 120"));
}

void PosixPtyBackendTest::interruptsForegroundCommand()
{
    auto backend = createTerminalBackend();
    QByteArray output;
    connect(backend.get(), &TerminalBackend::outputReceived, this, [&output](const QByteArray &data) {
        output += data;
    });
    QSignalSpy exited(backend.get(), &TerminalBackend::exited);

    QVERIFY(backend->start(shellProfile(QStringLiteral(
                               "trap 'exit 130' INT; printf ready; while :; do sleep 1; done")),
                           QDir::homePath(),
                           QSize(80, 24)));
    QTRY_VERIFY_WITH_TIMEOUT(output.contains("ready"), 3000);
    backend->interrupt();

    QTRY_COMPARE_WITH_TIMEOUT(exited.count(), 1, 5000);
    QCOMPARE(exited.first().at(0).toInt(), 130);
}

void PosixPtyBackendTest::destructionReapsChild()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString pidPath = directory.filePath(QStringLiteral("child.pid"));
    auto backend = createTerminalBackend();
    QVERIFY(backend->start(shellProfile(QStringLiteral("echo $$ > '%1'; while :; do sleep 1; done").arg(pidPath)),
                           directory.path(),
                           QSize(80, 24)));
    QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pidPath), 3000);

    QFile pidFile(pidPath);
    QVERIFY(pidFile.open(QIODevice::ReadOnly));
    bool ok = false;
    const pid_t pid = pidFile.readAll().trimmed().toLongLong(&ok);
    QVERIFY(ok);
    backend.reset();

    QTRY_VERIFY_WITH_TIMEOUT(::kill(pid, 0) == -1 && errno == ESRCH, 3000);
}

QTEST_GUILESS_MAIN(PosixPtyBackendTest)
#include "posixptybackend_test.moc"
