// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../quicktourdialog.h"

#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

using Zeal::WidgetUi::QuickTourDialog;

class QuickTourDialogTest : public QObject
{
    Q_OBJECT

private slots:
    void exposesCompleteTourAndActions();
};

void QuickTourDialogTest::exposesCompleteTourAndActions()
{
    QuickTourDialog dialog;
    QCOMPARE(dialog.pageIds().size(), 7);
    QVERIFY(dialog.doNotShowAutomatically());

    QSignalSpy docsetsSpy(&dialog, &QuickTourDialog::openDocsetLibraryRequested);
    QSignalSpy startNoteSpy(&dialog, &QuickTourDialog::openStartNoteRequested);

    auto *docsetsButton = dialog.findChild<QPushButton *>(QStringLiteral("openDocsetLibraryButton"));
    auto *startNoteButton = dialog.findChild<QPushButton *>(QStringLiteral("openStartNoteButton"));
    QVERIFY(docsetsButton);
    QVERIFY(startNoteButton);

    docsetsButton->click();
    startNoteButton->click();
    QCOMPARE(docsetsSpy.count(), 1);
    QCOMPARE(startNoteSpy.count(), 1);
}

QTEST_MAIN(QuickTourDialogTest)
#include "quicktourdialog_test.moc"
