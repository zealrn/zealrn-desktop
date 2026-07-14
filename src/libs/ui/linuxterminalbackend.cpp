// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QTimer>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <memory>
#include <vector>

namespace Zeal::WidgetUi {

namespace {

class PosixPtyBackend final : public TerminalBackend
{
public:
    explicit PosixPtyBackend(QObject *parent)
        : TerminalBackend(parent)
    {
        m_reapTimer.setInterval(20);
        connect(&m_reapTimer, &QTimer::timeout, this, &PosixPtyBackend::reapChild);
    }

    ~PosixPtyBackend() override { stopAndWait(false); }

    bool isAvailable() const override { return true; }
    QString unavailableReason() const override { return {}; }
    bool isRunning() const override { return m_pid > 0; }

    bool start(const TerminalProfile &profile, const QString &workingDirectory, QSize initialSize) override
    {
        if (!QFileInfo(profile.program).isExecutable() || !QFileInfo(workingDirectory).isDir()) {
            emit errorOccurred(tr("The selected shell or working directory is no longer available."));
            return false;
        }
        stopAndWait(true);

        std::vector<QByteArray> argumentStorage;
        argumentStorage.reserve(static_cast<size_t>(profile.arguments.size() + 1));
        argumentStorage.push_back(QFile::encodeName(profile.program));
        for (const QString &argument : profile.arguments) {
            argumentStorage.push_back(argument.toLocal8Bit());
        }
        std::vector<char *> arguments;
        arguments.reserve(argumentStorage.size() + 1);
        for (QByteArray &argument : argumentStorage) {
            arguments.push_back(argument.data());
        }
        arguments.push_back(nullptr);

        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("TERM"), QStringLiteral("xterm-256color"));
        environment.insert(QStringLiteral("COLORTERM"), QStringLiteral("truecolor"));
        const QStringList environmentList = environment.toStringList();
        std::vector<QByteArray> environmentStorage;
        environmentStorage.reserve(static_cast<size_t>(environmentList.size()));
        for (const QString &entry : environmentList) {
            environmentStorage.push_back(entry.toLocal8Bit());
        }
        std::vector<char *> environmentPointers;
        environmentPointers.reserve(environmentStorage.size() + 1);
        for (QByteArray &entry : environmentStorage) {
            environmentPointers.push_back(entry.data());
        }
        environmentPointers.push_back(nullptr);

        const QByteArray program = QFile::encodeName(profile.program);
        const QByteArray directory = QFile::encodeName(QDir(workingDirectory).absolutePath());
        struct winsize windowSize = {};
        windowSize.ws_col = static_cast<unsigned short>(qBound(1, initialSize.width(), 65535));
        windowSize.ws_row = static_cast<unsigned short>(qBound(1, initialSize.height(), 65535));

        int masterFd = -1;
        const pid_t pid = ::forkpty(&masterFd, nullptr, nullptr, &windowSize);
        if (pid < 0) {
            emit errorOccurred(tr("Could not create a terminal session: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            return false;
        }
        if (pid == 0) {
            if (::chdir(directory.constData()) != 0) {
                _exit(126);
            }
            ::execve(program.constData(), arguments.data(), environmentPointers.data());
            _exit(errno == ENOENT ? 127 : 126);
        }

        m_masterFd = masterFd;
        m_pid = pid;
        m_profile = profile;
        const int flags = ::fcntl(m_masterFd, F_GETFL, 0);
        if (flags >= 0) {
            ::fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);
        }
        ::fcntl(m_masterFd, F_SETFD, FD_CLOEXEC);

        m_readNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Read, this);
        m_writeNotifier = new QSocketNotifier(m_masterFd, QSocketNotifier::Write, this);
        m_writeNotifier->setEnabled(false);
        connect(m_readNotifier, &QSocketNotifier::activated, this, [this]() { readAvailable(); });
        connect(m_writeNotifier, &QSocketNotifier::activated, this, [this]() { flushWrites(); });
        m_reapTimer.start();
        emit started(m_profile);
        return true;
    }

    void write(const QByteArray &data) override
    {
        if (m_masterFd < 0 || data.isEmpty()) {
            return;
        }
        m_pendingWrites += data;
        flushWrites();
    }

    void resize(QSize size) override
    {
        if (m_masterFd < 0 || size.width() <= 0 || size.height() <= 0) {
            return;
        }
        struct winsize windowSize = {};
        windowSize.ws_col = static_cast<unsigned short>(qBound(1, size.width(), 65535));
        windowSize.ws_row = static_cast<unsigned short>(qBound(1, size.height(), 65535));
        if (::ioctl(m_masterFd, TIOCSWINSZ, &windowSize) == -1) {
            emit errorOccurred(tr("Could not resize the terminal: %1").arg(QString::fromLocal8Bit(strerror(errno))));
        }
    }

    void interrupt() override { write(QByteArray(1, '\x03')); }

    void terminate() override
    {
        if (m_pid <= 0) {
            return;
        }
        signalProcessGroups(SIGHUP);
        const pid_t terminatingPid = m_pid;
        QTimer::singleShot(250, this, [this, terminatingPid]() {
            if (m_pid == terminatingPid) {
                signalProcessGroups(SIGTERM);
            }
        });
        QTimer::singleShot(1250, this, [this, terminatingPid]() {
            if (m_pid == terminatingPid) {
                signalProcessGroups(SIGKILL);
            }
        });
    }

private:
    void readAvailable()
    {
        char buffer[16384];
        while (m_masterFd >= 0) {
            const ssize_t count = ::read(m_masterFd, buffer, sizeof(buffer));
            if (count > 0) {
                emit outputReceived(QByteArray(buffer, count));
                continue;
            }
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                return;
            }
            if (count < 0 && errno != EIO) {
                emit errorOccurred(tr("Could not read terminal output: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            }
            if (m_readNotifier) {
                m_readNotifier->setEnabled(false);
            }
            return;
        }
    }

    void flushWrites()
    {
        while (m_masterFd >= 0 && !m_pendingWrites.isEmpty()) {
            const ssize_t count = ::write(m_masterFd, m_pendingWrites.constData(), m_pendingWrites.size());
            if (count > 0) {
                m_pendingWrites.remove(0, count);
                continue;
            }
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                m_writeNotifier->setEnabled(true);
                return;
            }
            emit errorOccurred(tr("Could not send terminal input: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            m_pendingWrites.clear();
            break;
        }
        if (m_writeNotifier) {
            m_writeNotifier->setEnabled(false);
        }
    }

    void reapChild()
    {
        if (m_pid <= 0) {
            m_reapTimer.stop();
            return;
        }
        int status = 0;
        const pid_t result = ::waitpid(m_pid, &status, WNOHANG);
        if (result == 0 || (result < 0 && errno == EINTR)) {
            return;
        }
        if (result < 0) {
            emit errorOccurred(tr("Could not collect the terminal process: %1").arg(QString::fromLocal8Bit(strerror(errno))));
            cleanup();
            return;
        }
        finish(status, true);
    }

    void signalProcessGroups(int signal)
    {
        if (m_masterFd >= 0) {
            const pid_t foreground = ::tcgetpgrp(m_masterFd);
            if (foreground > 0) {
                ::kill(-foreground, signal);
            }
        }
        if (m_pid > 0) {
            ::kill(-m_pid, signal);
        }
    }

    void stopAndWait(bool notify)
    {
        if (m_pid <= 0) {
            cleanup();
            return;
        }

        signalProcessGroups(SIGHUP);
        int status = 0;
        pid_t result = 0;
        for (int attempt = 0; attempt < 20 && result == 0; ++attempt) {
            result = ::waitpid(m_pid, &status, WNOHANG);
            if (result == 0) {
                ::usleep(10000);
            } else if (result < 0 && errno == EINTR) {
                result = 0;
            }
        }
        if (result == 0) {
            signalProcessGroups(SIGTERM);
            for (int attempt = 0; attempt < 80 && result == 0; ++attempt) {
                result = ::waitpid(m_pid, &status, WNOHANG);
                if (result == 0) {
                    ::usleep(10000);
                } else if (result < 0 && errno == EINTR) {
                    result = 0;
                }
            }
        }
        if (result == 0) {
            signalProcessGroups(SIGKILL);
            do {
                result = ::waitpid(m_pid, &status, 0);
            } while (result < 0 && errno == EINTR);
        }

        if (result > 0) {
            finish(status, notify);
        } else {
            cleanup();
        }
    }

    void finish(int status, bool notify)
    {
        int exitCode = 0;
        bool exitCodeAvailable = false;
        if (WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
            exitCodeAvailable = true;
        } else if (WIFSIGNALED(status)) {
            exitCode = 128 + WTERMSIG(status);
            exitCodeAvailable = true;
        }
        cleanup();
        if (notify) {
            emit exited(exitCode, exitCodeAvailable);
        }
    }

    void cleanup()
    {
        m_reapTimer.stop();
        delete m_readNotifier;
        m_readNotifier = nullptr;
        delete m_writeNotifier;
        m_writeNotifier = nullptr;
        if (m_masterFd >= 0) {
            ::close(m_masterFd);
            m_masterFd = -1;
        }
        m_pid = -1;
        m_pendingWrites.clear();
    }

    int m_masterFd = -1;
    pid_t m_pid = -1;
    QSocketNotifier *m_readNotifier = nullptr;
    QSocketNotifier *m_writeNotifier = nullptr;
    QTimer m_reapTimer;
    QByteArray m_pendingWrites;
    TerminalProfile m_profile;
};

} // namespace

std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent)
{
    return std::make_unique<PosixPtyBackend>(parent);
}

} // namespace Zeal::WidgetUi
