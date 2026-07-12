// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../learningnotespanel.h"
#include "../learningnotesstore.h"

#include <QLabel>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTest>

using namespace Zeal::WidgetUi;

class LearningNotesPanelTest : public QObject
{
    Q_OBJECT

private slots:
    void pageChange_loadsMatchingNote();
    void edit_autosavesAndUpdatesStatus();
    void pageChange_flushesPendingEdit();
    void selection_appendsBlockquoteOnce();
};

static LearningNotePage page(const QString &path)
{
    return {.docsetId = QStringLiteral("Qt_6"),
            .docsetName = QStringLiteral("Qt 6"),
            .pageKey = path,
            .pagePath = path,
            .pageUrl = QStringLiteral("http://localhost/Qt_6/") + path,
            .pageTitle = path};
}

void LearningNotesPanelTest::pageChange_loadsMatchingNote()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("notes.sqlite"));
    {
        LearningNotesStore store(databasePath);
        LearningNote note;
        note.page = page(QStringLiteral("qwidget.html"));
        note.content = QStringLiteral("Widget note");
        QVERIFY(store.save(&note));
    }

    LearningNotesPanel panel(databasePath);
    QVERIFY(panel.setPage(page(QStringLiteral("qwidget.html"))));
    QCOMPARE(panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"))->toPlainText(),
             QStringLiteral("Widget note"));
    QCOMPARE(panel.findChild<QLabel *>(QStringLiteral("noteStatus"))->text(), QStringLiteral("Saved"));

    QVERIFY(panel.setPage(page(QStringLiteral("qobject.html"))));
    QVERIFY(panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"))->toPlainText().isEmpty());
    QCOMPARE(panel.findChild<QLabel *>(QStringLiteral("noteStatus"))->text(), QStringLiteral("New note"));
}

void LearningNotesPanelTest::edit_autosavesAndUpdatesStatus()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString databasePath = dir.filePath(QStringLiteral("notes.sqlite"));
    LearningNotesPanel panel(databasePath);
    QVERIFY(panel.setPage(page(QStringLiteral("qsqlquery.html"))));

    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    auto *status = panel.findChild<QLabel *>(QStringLiteral("noteStatus"));
    editor->setPlainText(QStringLiteral("Bind every value."));
    QCOMPARE(status->text(), QStringLiteral("Unsaved"));
    QTRY_COMPARE_WITH_TIMEOUT(status->text(), QStringLiteral("Saved"), 1500);

    LearningNotesStore store(databasePath);
    const auto saved = store.note(QStringLiteral("Qt_6"), QStringLiteral("qsqlquery.html"));
    QVERIFY(saved.has_value());
    QCOMPARE(saved->content, QStringLiteral("Bind every value."));
}

void LearningNotesPanelTest::pageChange_flushesPendingEdit()
{
    QTemporaryDir dir;
    const QString databasePath = dir.filePath(QStringLiteral("notes.sqlite"));
    LearningNotesPanel panel(databasePath);
    const LearningNotePage firstPage = page(QStringLiteral("first.html"));
    QVERIFY(panel.setPage(firstPage));
    panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"))->setPlainText(QStringLiteral("Pending"));

    QVERIFY(panel.setPage(page(QStringLiteral("second.html"))));

    LearningNotesStore store(databasePath);
    const auto saved = store.note(firstPage.docsetId, firstPage.pageKey);
    QVERIFY(saved.has_value());
    QCOMPARE(saved->content, QStringLiteral("Pending"));
}

void LearningNotesPanelTest::selection_appendsBlockquoteOnce()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(panel.setPage(page(QStringLiteral("selection.html"))));
    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    editor->setPlainText(QStringLiteral("Existing"));

    panel.appendSelection(QStringLiteral("first line\nsecond line"));
    panel.appendSelection(QStringLiteral("first line\nsecond line"));

    QCOMPARE(editor->toPlainText(), QStringLiteral("Existing\n\n> first line\n> second line"));
}

QTEST_MAIN(LearningNotesPanelTest)
#include "learningnotespanel_test.moc"
