// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../learningnotespanel.h"
#include "../learningnotesstore.h"

#include <core/settings.h>

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTextBrowser>
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
    void formatting_preservesUndoHistory();
    void preview_preservesEditorDocument();
    void preview_doesNotRenderRawHtml();
    void countsAndFind_followEditor();
    void zoomAndLineWrap_followSettings();
    void layoutActions_emitRequestedModes();
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

void LearningNotesPanelTest::formatting_preservesUndoHistory()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(panel.setPage(page(QStringLiteral("formatting.html"))));
    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    editor->insertPlainText(QStringLiteral("hello"));
    editor->selectAll();

    panel.findChild<QAction *>(QStringLiteral("noteBoldAction"))->trigger();

    QCOMPARE(editor->toPlainText(), QStringLiteral("**hello**"));
    QVERIFY(editor->document()->isUndoAvailable());
    editor->undo();
    QCOMPARE(editor->toPlainText(), QStringLiteral("hello"));
}

void LearningNotesPanelTest::preview_preservesEditorDocument()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(panel.setPage(page(QStringLiteral("preview.html"))));
    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    editor->insertPlainText(QStringLiteral("# Preview title"));
    QVERIFY(editor->document()->isUndoAvailable());

    panel.findChild<QTabWidget *>(QStringLiteral("noteModeTabs"))->setCurrentIndex(1);

    QCOMPARE(editor->toPlainText(), QStringLiteral("# Preview title"));
    QVERIFY(editor->document()->isUndoAvailable());
    QVERIFY(panel.findChild<QTextBrowser *>(QStringLiteral("notePreview"))->toPlainText().contains(
        QStringLiteral("Preview title")));
}

void LearningNotesPanelTest::preview_doesNotRenderRawHtml()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(panel.setPage(page(QStringLiteral("unsafe-preview.html"))));
    panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"))
        ->setPlainText(QStringLiteral("<script>alert('unsafe')</script>\n<img src=\"file:///etc/passwd\">"));

    panel.findChild<QTabWidget *>(QStringLiteral("noteModeTabs"))->setCurrentIndex(1);
    const QString rendered = panel.findChild<QTextBrowser *>(QStringLiteral("notePreview"))->toHtml();
    QVERIFY(!rendered.contains(QStringLiteral("<script"), Qt::CaseInsensitive));
    QVERIFY(!rendered.contains(QStringLiteral("<img"), Qt::CaseInsensitive));
}

void LearningNotesPanelTest::countsAndFind_followEditor()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QVERIFY(panel.setPage(page(QStringLiteral("find.html"))));
    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    editor->setPlainText(QStringLiteral("hello world"));
    QCOMPARE(panel.findChild<QLabel *>(QStringLiteral("noteCount"))->text(),
             QStringLiteral("2 words · 11 characters"));

    panel.findChild<QAction *>(QStringLiteral("noteFindAction"))->trigger();
    auto *findEdit = panel.findChild<QLineEdit *>(QStringLiteral("noteFindEdit"));
    findEdit->setText(QStringLiteral("world"));
    panel.findChild<QAction *>(QStringLiteral("noteFindNextAction"))->trigger();
    QCOMPARE(editor->textCursor().selectedText(), QStringLiteral("world"));
    QCOMPARE(panel.findChild<QLabel *>(QStringLiteral("noteCount"))->text(),
             QStringLiteral("2 words · 11 characters · 5 selected"));
}

void LearningNotesPanelTest::zoomAndLineWrap_followSettings()
{
    QTemporaryDir settingsDir;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir.path());
    QCoreApplication::setOrganizationName(QStringLiteral("LearningNotesPanelTest"));
    QCoreApplication::setApplicationName(QStringLiteral("LearningNotesPanelTest"));
    Zeal::Core::Settings settings;
    settings.learningNotesZoom = 150;
    settings.learningNotesLineWrap = false;

    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")), &settings);
    auto *editor = panel.findChild<QPlainTextEdit *>(QStringLiteral("noteEditor"));
    QCOMPARE(panel.findChild<QLabel *>(QStringLiteral("noteZoom"))->text(), QStringLiteral("150%"));
    QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);

    panel.findChild<QAction *>(QStringLiteral("noteZoomInAction"))->trigger();
    QCOMPARE(settings.learningNotesZoom, 160);
}

void LearningNotesPanelTest::layoutActions_emitRequestedModes()
{
    QTemporaryDir dir;
    LearningNotesPanel panel(dir.filePath(QStringLiteral("notes.sqlite")));
    QSignalSpy expanded(&panel, &LearningNotesPanel::expandedModeRequested);
    QSignalSpy focused(&panel, &LearningNotesPanel::focusModeRequested);

    panel.findChild<QAction *>(QStringLiteral("noteExpandAction"))->trigger();
    panel.findChild<QAction *>(QStringLiteral("noteFocusAction"))->trigger();

    QCOMPARE(expanded.count(), 1);
    QCOMPARE(expanded.takeFirst().constFirst().toBool(), true);
    QCOMPARE(focused.count(), 1);
    QCOMPARE(focused.takeFirst().constFirst().toBool(), true);

    panel.exitFocusMode();
    panel.exitExpandedMode();
    QCOMPARE(focused.takeFirst().constFirst().toBool(), false);
    QCOMPARE(expanded.takeFirst().constFirst().toBool(), false);
}

QTEST_MAIN(LearningNotesPanelTest)
#include "learningnotespanel_test.moc"
