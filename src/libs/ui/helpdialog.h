// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_HELPDIALOG_H
#define ZEAL_WIDGETUI_HELPDIALOG_H

#include <QDialog>
#include <QUrl>

class QTabWidget;

namespace Zeal::WidgetUi {

class HelpDialog final : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(HelpDialog)

public:
    enum class Section {
        GettingStarted,
        KeyboardShortcuts,
        NotesAndBackups,
        WebPlayground,
        DeveloperTerminal,
        DocsetLibrary,
        ReportProblem,
    };

    explicit HelpDialog(Section section, const QString &terminalBackend, int schemaVersion, int docsetCount,
                        QWidget *parent = nullptr);

    static QUrl reportIssueUrl();
    static QString diagnosticReport(const QString &terminalBackend, int schemaVersion, int docsetCount);
    static QString keyboardShortcutsText();

private:
    QTabWidget *m_tabs = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_HELPDIALOG_H
