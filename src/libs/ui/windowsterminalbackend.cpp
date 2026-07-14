// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

#include "terminalsupport.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QWinEventNotifier>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <condition_variable>
#include <cstdio>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace Zeal::WidgetUi {

namespace {

constexpr qsizetype MaxPendingInput = 4 * 1024 * 1024;
constexpr DWORD MinimumConPtyBuild = 26100;

DWORD currentWindowsBuild()
{
    using RtlGetVersionFunction = LONG(WINAPI *)(OSVERSIONINFOW *);
    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFunction>(
        GetProcAddress(ntdll, "RtlGetVersion"));
    OSVERSIONINFOW version = {};
    version.dwOSVersionInfoSize = sizeof(version);
    return rtlGetVersion && rtlGetVersion(&version) == 0 ? version.dwBuildNumber : 0;
}

QString windowsErrorMessage(DWORD error)
{
    wchar_t *message = nullptr;
    const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                                            | FORMAT_MESSAGE_IGNORE_INSERTS,
                                        nullptr,
                                        error,
                                        0,
                                        reinterpret_cast<wchar_t *>(&message),
                                        0,
                                        nullptr);
    const QString result = length > 0 ? QString::fromWCharArray(message, length).trimmed()
                                      : QStringLiteral("Windows error %1").arg(error);
    LocalFree(message);
    return result;
}

QString conPtyErrorMessage(HRESULT result)
{
    return windowsErrorMessage(HRESULT_FACILITY(result) == FACILITY_WIN32 ? HRESULT_CODE(result)
                                                                          : static_cast<DWORD>(result));
}

struct ConPtyApi
{
    using CreateFunction = HRESULT(WINAPI *)(COORD, HANDLE, HANDLE, DWORD, HPCON *);
    using ResizeFunction = HRESULT(WINAPI *)(HPCON, COORD);
    using CloseFunction = void(WINAPI *)(HPCON);

    ConPtyApi()
    {
        const HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
        create = reinterpret_cast<CreateFunction>(GetProcAddress(kernel, "CreatePseudoConsole"));
        resize = reinterpret_cast<ResizeFunction>(GetProcAddress(kernel, "ResizePseudoConsole"));
        close = reinterpret_cast<CloseFunction>(GetProcAddress(kernel, "ClosePseudoConsole"));
        build = currentWindowsBuild();
    }

    [[nodiscard]] bool hasFunctions() const { return create && resize && close; }
    [[nodiscard]] bool isAvailable() const { return hasFunctions() && build >= MinimumConPtyBuild; }

    CreateFunction create = nullptr;
    ResizeFunction resize = nullptr;
    CloseFunction close = nullptr;
    DWORD build = 0;
};

class WindowsConPtyBackend final : public TerminalBackend
{
public:
    explicit WindowsConPtyBackend(QObject *parent = nullptr)
        : TerminalBackend(parent)
    {
    }

    ~WindowsConPtyBackend() override { shutdown(false); }

    bool isAvailable() const override { return m_api.isAvailable(); }
    QString unavailableReason() const override
    {
        if (!m_api.hasFunctions()) {
            return tr("Windows ConPTY is unavailable. Use Open External Terminal.");
        }
        return tr("The embedded terminal requires Windows 11 version 24H2 or later. Use Open External Terminal.");
    }
    bool isRunning() const override { return m_process != nullptr; }

    bool start(const TerminalProfile &profile, const QString &workingDirectory, QSize initialSize) override
    {
        if (!isAvailable()) {
            emit errorOccurred(unavailableReason());
            return false;
        }
        if (!QFileInfo(profile.program).isExecutable() || !QFileInfo(workingDirectory).isDir()) {
            emit errorOccurred(tr("The selected shell or working directory is no longer available."));
            return false;
        }
        shutdown(true);

        HANDLE inputRead = nullptr;
        HANDLE outputWrite = nullptr;
        if (!CreatePipe(&inputRead, &m_inputWrite, nullptr, 0)
            || !CreatePipe(&m_outputRead, &outputWrite, nullptr, 0)) {
            const QString error = windowsErrorMessage(GetLastError());
            closeHandle(inputRead);
            closeHandle(outputWrite);
            closeIoHandles();
            emit errorOccurred(tr("Could not create terminal pipes: %1").arg(error));
            return false;
        }

        const COORD size = terminalSize(initialSize);
        const HRESULT pseudoConsoleResult = m_api.create(size, inputRead, outputWrite, 0, &m_pseudoConsole);
        if (FAILED(pseudoConsoleResult)) {
            closeHandle(inputRead);
            closeHandle(outputWrite);
            closeIoHandles();
            emit errorOccurred(tr("Could not create a Windows terminal: %1").arg(conPtyErrorMessage(pseudoConsoleResult)));
            return false;
        }

        SIZE_T attributeBytes = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeBytes);
        auto *attributes = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, attributeBytes));
        if (!attributes || !InitializeProcThreadAttributeList(attributes, 1, 0, &attributeBytes)
            || !UpdateProcThreadAttribute(attributes,
                                          0,
                                          PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                          m_pseudoConsole,
                                          sizeof(m_pseudoConsole),
                                          nullptr,
                                          nullptr)) {
            const QString error = windowsErrorMessage(GetLastError());
            if (attributes) {
                DeleteProcThreadAttributeList(attributes);
                HeapFree(GetProcessHeap(), 0, attributes);
            }
            closeHandle(inputRead);
            closeHandle(outputWrite);
            closeIoHandles();
            closePseudoConsole();
            emit errorOccurred(tr("Could not prepare the terminal process: %1").arg(error));
            return false;
        }

        STARTUPINFOEXW startup = {};
        startup.StartupInfo.cb = sizeof(startup);
        startup.lpAttributeList = attributes;
        const std::wstring application = QDir::toNativeSeparators(profile.program).toStdWString();
        std::wstring commandLine = TerminalSupport::windowsCommandLine(profile).toStdWString();
        std::vector<wchar_t> mutableCommandLine(commandLine.begin(), commandLine.end());
        mutableCommandLine.push_back(L'\0');
        const std::wstring directory = QDir::toNativeSeparators(workingDirectory).toStdWString();
        PROCESS_INFORMATION process = {};
        const BOOL created = CreateProcessW(application.c_str(),
                                            mutableCommandLine.data(),
                                            nullptr,
                                            nullptr,
                                            FALSE,
                                            EXTENDED_STARTUPINFO_PRESENT | CREATE_UNICODE_ENVIRONMENT
                                                | CREATE_SUSPENDED,
                                            nullptr,
                                            directory.c_str(),
                                            &startup.StartupInfo,
                                            &process);
        const DWORD createError = created ? ERROR_SUCCESS : GetLastError();
        closeHandle(inputRead);
        closeHandle(outputWrite);
        DeleteProcThreadAttributeList(attributes);
        HeapFree(GetProcessHeap(), 0, attributes);
        if (!created) {
            closeIoHandles();
            closePseudoConsole();
            emit errorOccurred(tr("Could not start the selected shell: %1").arg(windowsErrorMessage(createError)));
            return false;
        }

        m_process = process.hProcess;
        m_job = CreateJobObjectW(nullptr, nullptr);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
        limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!m_job
            || !SetInformationJobObject(m_job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))
            || !AssignProcessToJobObject(m_job, m_process)) {
            const QString error = windowsErrorMessage(GetLastError());
            TerminateProcess(m_process, 1);
            WaitForSingleObject(m_process, 2000);
            CloseHandle(process.hThread);
            closeHandle(m_process);
            closeHandle(m_job);
            closeIoHandles();
            closePseudoConsole();
            emit errorOccurred(tr("Could not contain the terminal process tree: %1").arg(error));
            return false;
        }
        if (ResumeThread(process.hThread) == DWORD(-1)) {
            const QString error = windowsErrorMessage(GetLastError());
            CloseHandle(process.hThread);
            shutdown(false);
            emit errorOccurred(tr("Could not resume the selected shell: %1").arg(error));
            return false;
        }
        CloseHandle(process.hThread);

        startIoThreads();
        m_processNotifier = new QWinEventNotifier(m_process, this);
        connect(m_processNotifier, &QWinEventNotifier::activated, this, [this]() {
            std::fprintf(stderr, "[conpty] process signaled\n");
            std::fflush(stderr);
            DWORD exitCode = 0;
            GetExitCodeProcess(m_process, &exitCode);
            finish(static_cast<int>(exitCode), true);
        });
        emit started(profile);
        return true;
    }

    void write(const QByteArray &data) override
    {
        if (!m_process || data.isEmpty()) {
            return;
        }
        std::fprintf(stderr, "[conpty] queue input: %lld bytes\n", static_cast<long long>(data.size()));
        std::fflush(stderr);
        {
            std::lock_guard lock(m_writeMutex);
            if (m_pendingInput.size() + data.size() > MaxPendingInput) {
                emit errorOccurred(tr("Terminal input is too large."));
                return;
            }
            m_pendingInput += data;
        }
        m_writeReady.notify_one();
    }

    void resize(QSize size) override
    {
        if (size.width() <= 0 || size.height() <= 0) {
            return;
        }
        std::lock_guard lock(m_pseudoConsoleMutex);
        if (!m_pseudoConsole) {
            return;
        }
        const HRESULT result = m_api.resize(m_pseudoConsole, terminalSize(size));
        if (FAILED(result)) {
            emit errorOccurred(tr("Could not resize the terminal: %1").arg(conPtyErrorMessage(result)));
        }
    }

    void interrupt() override { write(QByteArray(1, '\x03')); }

    void terminate() override
    {
        if (m_job) {
            TerminateJobObject(m_job, 1);
        } else if (m_process) {
            TerminateProcess(m_process, 1);
        }
    }

private:
    static COORD terminalSize(QSize size)
    {
        return {static_cast<SHORT>(std::clamp(size.width(), 1, int(SHRT_MAX))),
                static_cast<SHORT>(std::clamp(size.height(), 1, int(SHRT_MAX)))};
    }

    static void closeHandle(HANDLE &handle)
    {
        if (handle) {
            CloseHandle(handle);
            handle = nullptr;
        }
    }

    void startIoThreads()
    {
        m_stopIo = false;
        const HANDLE output = m_outputRead;
        m_reader = std::thread([this, output]() {
            std::fprintf(stderr, "[conpty] reader started\n");
            std::fflush(stderr);
            char buffer[16384];
            while (!m_stopIo) {
                DWORD available = 0;
                if (!PeekNamedPipe(output, nullptr, 0, nullptr, &available, nullptr)) {
                    std::fprintf(stderr, "[conpty] PeekNamedPipe failed: %lu\n", GetLastError());
                    std::fflush(stderr);
                    return;
                }
                if (available == 0) {
                    // ponytail: polling avoids uncancellable reads; use overlapped pipes if idle wakeups matter.
                    Sleep(5);
                    continue;
                }
                DWORD count = 0;
                const DWORD requested = std::min<DWORD>(available, sizeof(buffer));
                if (!ReadFile(output, buffer, requested, &count, nullptr)) {
                    std::fprintf(stderr, "[conpty] ReadFile failed: %lu\n", GetLastError());
                    std::fflush(stderr);
                    return;
                }
                const QByteArray data(buffer, static_cast<qsizetype>(count));
                const bool queued = QMetaObject::invokeMethod(
                    this,
                    [this, data]() { emit outputReceived(data); },
                    Qt::QueuedConnection);
                std::fprintf(stderr,
                             "[conpty] read: %lu bytes; signal queued: %d\n",
                             count,
                             queued ? 1 : 0);
                std::fflush(stderr);
            }
        });

        const HANDLE input = m_inputWrite;
        m_writer = std::thread([this, input]() {
            std::fprintf(stderr, "[conpty] writer started\n");
            std::fflush(stderr);
            while (!m_stopIo) {
                QByteArray data;
                {
                    std::unique_lock lock(m_writeMutex);
                    m_writeReady.wait(lock, [this]() { return m_stopIo || !m_pendingInput.isEmpty(); });
                    if (m_stopIo) {
                        return;
                    }
                    data = std::move(m_pendingInput);
                    m_pendingInput.clear();
                }
                qsizetype offset = 0;
                while (!m_stopIo && offset < data.size()) {
                    DWORD written = 0;
                    const DWORD remaining = static_cast<DWORD>(
                        std::min<qsizetype>(data.size() - offset, std::numeric_limits<DWORD>::max()));
                    if (!WriteFile(input, data.constData() + offset, remaining, &written, nullptr)) {
                        const DWORD error = GetLastError();
                        std::fprintf(stderr, "[conpty] WriteFile failed: %lu\n", error);
                        std::fflush(stderr);
                        if (!m_stopIo) {
                            QMetaObject::invokeMethod(
                                this,
                                [this, error]() {
                                    emit errorOccurred(
                                        tr("Could not send terminal input: %1").arg(windowsErrorMessage(error)));
                                },
                                Qt::QueuedConnection);
                        }
                        return;
                    }
                    std::fprintf(stderr, "[conpty] wrote: %lu bytes\n", written);
                    std::fflush(stderr);
                    offset += written;
                }
            }
        });
    }

    void stopIoThreads()
    {
        m_stopIo = true;
        m_writeReady.notify_all();
        if (m_writer.joinable()) {
            CancelSynchronousIo(m_writer.native_handle());
        }
        if (m_reader.joinable()) {
            CancelSynchronousIo(m_reader.native_handle());
        }
        if (m_writer.joinable()) {
            m_writer.join();
        }
        if (m_reader.joinable()) {
            m_reader.join();
        }
        closeIoHandles();
        std::lock_guard lock(m_writeMutex);
        m_pendingInput.clear();
    }

    void closeIoHandles()
    {
        closeHandle(m_inputWrite);
        closeHandle(m_outputRead);
    }

    void closePseudoConsole()
    {
        const HPCON pseudoConsole = takePseudoConsole();
        if (pseudoConsole) {
            m_api.close(pseudoConsole);
        }
    }

    HPCON takePseudoConsole()
    {
        std::lock_guard lock(m_pseudoConsoleMutex);
        return std::exchange(m_pseudoConsole, nullptr);
    }

    void shutdown(bool notify)
    {
        if (!m_process) {
            stopIoThreads();
            return;
        }
        terminate();
        WaitForSingleObject(m_process, 2000);
        DWORD exitCode = 1;
        GetExitCodeProcess(m_process, &exitCode);
        finish(static_cast<int>(exitCode), notify);
    }

    void finish(int exitCode, bool notify)
    {
        if (m_processNotifier) {
            delete m_processNotifier;
            m_processNotifier = nullptr;
        }
        stopIoThreads();
        closeHandle(m_process);
        closeHandle(m_job);
        closePseudoConsole();
        if (notify) {
            emit exited(exitCode, true);
        }
    }

    ConPtyApi m_api;
    HPCON m_pseudoConsole = nullptr;
    std::mutex m_pseudoConsoleMutex;
    HANDLE m_inputWrite = nullptr;
    HANDLE m_outputRead = nullptr;
    HANDLE m_process = nullptr;
    HANDLE m_job = nullptr;
    QWinEventNotifier *m_processNotifier = nullptr;
    std::thread m_reader;
    std::thread m_writer;
    std::atomic_bool m_stopIo = true;
    std::mutex m_writeMutex;
    std::condition_variable m_writeReady;
    QByteArray m_pendingInput;
};

} // namespace

std::unique_ptr<TerminalBackend> createTerminalBackend(QObject *parent)
{
    return std::make_unique<WindowsConPtyBackend>(parent);
}

} // namespace Zeal::WidgetUi
