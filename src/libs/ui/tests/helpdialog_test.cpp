// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../helpdialog.h"

#include <QDir>
#include <QtTest>

using Zeal::WidgetUi::HelpDialog;

class HelpDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void usesZealrnSupportIdentity();
    void diagnosticsExcludeSensitiveData();
    void listsImplementedShortcuts();
};

void HelpDialogTest::usesZealrnSupportIdentity()
{
    QCOMPARE(HelpDialog::reportIssueUrl().toString(),
             QStringLiteral("https://github.com/zealrn/zealrn-desktop/issues/new"));
}

void HelpDialogTest::diagnosticsExcludeSensitiveData()
{
    const QString report = HelpDialog::diagnosticReport(QStringLiteral("POSIX PTY"), 1, 4);
    QVERIFY(report.contains(QStringLiteral("ZealRN")));
    QVERIFY(report.contains(QStringLiteral("POSIX PTY")));
    QVERIFY(report.contains(QStringLiteral("Schema version: 1")));
    QVERIFY(report.contains(QStringLiteral("Docsets: 4")));
    QVERIFY(!report.contains(QDir::homePath()));
    const QString user = qEnvironmentVariable("USER");
    QVERIFY(user.isEmpty() || !report.contains(user));
}

void HelpDialogTest::listsImplementedShortcuts()
{
    const QString shortcuts = HelpDialog::keyboardShortcutsText();
    QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+S")));
    QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+`")));
    QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+B")));
    QVERIFY(shortcuts.contains(QStringLiteral("Ctrl+Shift+L")));
}

QTEST_MAIN(HelpDialogTest)
#include "helpdialog_test.moc"
