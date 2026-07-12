// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalsupport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>

namespace Zeal::WidgetUi::TerminalSupport {

namespace {

QString executableNamed(const QStringList &programs, const QString &name)
{
    for (const QString &program : programs) {
        if (QFileInfo(program).fileName().compare(name, Qt::CaseInsensitive) == 0) {
            return program;
        }
    }
    return {};
}

void appendExecutable(QStringList &shells, const QString &path)
{
    const QFileInfo file(path);
    if (!path.isEmpty() && file.isFile() && file.isExecutable() && !shells.contains(file.absoluteFilePath())) {
        shells.append(file.absoluteFilePath());
    }
}

} // namespace

QStringList availableShells()
{
    QStringList shells;

#ifdef Q_OS_WINDOWS
    for (const QString &name : {QStringLiteral("pwsh.exe"), QStringLiteral("powershell.exe"), QStringLiteral("cmd.exe")}) {
        appendExecutable(shells, QStandardPaths::findExecutable(name));
    }
#else
    appendExecutable(shells, QProcessEnvironment::systemEnvironment().value(QStringLiteral("SHELL")));

    QFile file(QStringLiteral("/etc/shells"));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString path = stream.readLine().trimmed();
            if (!path.startsWith(QLatin1Char('#'))) {
                appendExecutable(shells, path);
            }
        }
    }

    for (const QString &name : {QStringLiteral("bash"), QStringLiteral("zsh"), QStringLiteral("fish"), QStringLiteral("sh")}) {
        appendExecutable(shells, QStandardPaths::findExecutable(name));
    }
#endif

    return shells;
}

QString validatedShell(const QString &savedShell, const QStringList &shells)
{
    return shells.contains(savedShell) ? savedShell : shells.value(0);
}

QString validatedWorkingDirectory(const QString &savedDirectory,
                                  const QString &homeDirectory,
                                  const QString &workspaceDirectory)
{
    for (const QString &path : {savedDirectory, homeDirectory, workspaceDirectory}) {
        if (!path.isEmpty() && QDir(path).exists()) {
            return QDir(path).absolutePath();
        }
    }
    return {};
}

BottomTool bottomToolFromValue(int value)
{
    return value == static_cast<int>(BottomTool::DeveloperTerminal) ? BottomTool::DeveloperTerminal
                                                                    : BottomTool::WebPlayground;
}

ExternalTerminalLaunch externalTerminalLaunch(Platform platform,
                                              const QStringList &availablePrograms,
                                              const QString &shell,
                                              const QString &workingDirectory)
{
    if (platform == Platform::Windows) {
        if (const QString terminal = executableNamed(availablePrograms, QStringLiteral("wt.exe")); !terminal.isEmpty()) {
            return {terminal, {QStringLiteral("-d"), workingDirectory, shell}};
        }

        if (availablePrograms.contains(shell)) {
            return {shell, {}};
        }

        for (const QString &name : {QStringLiteral("pwsh.exe"), QStringLiteral("powershell.exe"), QStringLiteral("cmd.exe")}) {
            if (const QString terminal = executableNamed(availablePrograms, name); !terminal.isEmpty()) {
                return {terminal, {}};
            }
        }
        return {};
    }

    if (platform != Platform::Linux) {
        return {};
    }

    struct Candidate {
        const char *name;
        QStringList arguments;
    };
    const QList<Candidate> candidates = {
        {"xdg-terminal-exec", {shell}},
        {"gnome-terminal", {QStringLiteral("--working-directory=%1").arg(workingDirectory), QStringLiteral("--"), shell}},
        {"konsole", {QStringLiteral("--workdir"), workingDirectory, QStringLiteral("-e"), shell}},
        {"xfce4-terminal", {QStringLiteral("--working-directory"), workingDirectory, QStringLiteral("-x"), shell}},
        {"kitty", {QStringLiteral("--directory"), workingDirectory, shell}},
        {"alacritty", {QStringLiteral("--working-directory"), workingDirectory, QStringLiteral("-e"), shell}},
        {"wezterm", {QStringLiteral("start"), QStringLiteral("--cwd"), workingDirectory, QStringLiteral("--"), shell}},
        {"xterm", {QStringLiteral("-e"), shell}},
    };

    for (const Candidate &candidate : candidates) {
        if (const QString terminal = executableNamed(availablePrograms, QString::fromLatin1(candidate.name));
            !terminal.isEmpty()) {
            return {terminal, candidate.arguments};
        }
    }
    return {};
}

ExternalTerminalLaunch detectedExternalTerminal(const QString &shell, const QString &workingDirectory)
{
#ifdef Q_OS_WINDOWS
    const Platform platform = Platform::Windows;
    const QStringList names = {QStringLiteral("wt.exe"),
                               QStringLiteral("pwsh.exe"),
                               QStringLiteral("powershell.exe"),
                               QStringLiteral("cmd.exe")};
#elif defined(Q_OS_LINUX)
    const Platform platform = Platform::Linux;
    const QStringList names = {QStringLiteral("xdg-terminal-exec"),
                               QStringLiteral("gnome-terminal"),
                               QStringLiteral("konsole"),
                               QStringLiteral("xfce4-terminal"),
                               QStringLiteral("kitty"),
                               QStringLiteral("alacritty"),
                               QStringLiteral("wezterm"),
                               QStringLiteral("xterm")};
#else
    const Platform platform = Platform::Unsupported;
    const QStringList names;
#endif

    QStringList programs;
    for (const QString &name : names) {
        const QString path = QStandardPaths::findExecutable(name);
        if (!path.isEmpty()) {
            programs.append(path);
        }
    }
    return externalTerminalLaunch(platform, programs, shell, workingDirectory);
}

} // namespace Zeal::WidgetUi::TerminalSupport
