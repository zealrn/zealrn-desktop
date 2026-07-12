// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_TERMINALSUPPORT_H
#define ZEAL_WIDGETUI_TERMINALSUPPORT_H

#include <QStringList>

namespace Zeal::WidgetUi::TerminalSupport {

enum class BottomTool {
    WebPlayground = 0,
    DeveloperTerminal = 1,
};

enum class Platform {
    Linux,
    Windows,
    Unsupported,
};

struct ExternalTerminalLaunch {
    QString program;
    QStringList arguments;

    [[nodiscard]] bool isValid() const { return !program.isEmpty(); }
};

QStringList availableShells();
QString validatedShell(const QString &savedShell, const QStringList &shells);
QString validatedWorkingDirectory(const QString &savedDirectory,
                                  const QString &homeDirectory,
                                  const QString &workspaceDirectory);
BottomTool bottomToolFromValue(int value);
ExternalTerminalLaunch externalTerminalLaunch(Platform platform,
                                              const QStringList &availablePrograms,
                                              const QString &shell,
                                              const QString &workingDirectory);
ExternalTerminalLaunch detectedExternalTerminal(const QString &shell, const QString &workingDirectory);

} // namespace Zeal::WidgetUi::TerminalSupport

#endif // ZEAL_WIDGETUI_TERMINALSUPPORT_H
