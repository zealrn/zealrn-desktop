// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../learningnotesmarkdown.h"

#include <QTest>

using namespace Zeal::WidgetUi::LearningNotesMarkdown;

class LearningNotesMarkdownTest : public QObject
{
    Q_OBJECT

private slots:
    void zoom_isClamped();
    void countsWordsCharactersAndSelection();
    void wrapsSelectedText();
    void insertsPlaceholderWithoutSelection();
    void prefixesEverySelectedLine();
    void createsNumberedList();
    void createsCodeBlockAndLink();
};

void LearningNotesMarkdownTest::zoom_isClamped()
{
    QCOMPARE(clampZoom(20), 80);
    QCOMPARE(clampZoom(115), 115);
    QCOMPARE(clampZoom(300), 200);
}

void LearningNotesMarkdownTest::countsWordsCharactersAndSelection()
{
    const Counts value = counts(QStringLiteral("Hello, brave\nnew world"), 7, 5);
    QCOMPARE(value.words, 4);
    QCOMPARE(value.characters, 22);
    QCOMPARE(value.selectedCharacters, 5);
}

void LearningNotesMarkdownTest::wrapsSelectedText()
{
    const Edit edit = apply(Action::Bold, QStringLiteral("Learn Qt"), 6, 2);
    QCOMPARE(edit.text, QStringLiteral("Learn **Qt**"));
    QCOMPARE(edit.selectionStart, 8);
    QCOMPARE(edit.selectionLength, 2);
}

void LearningNotesMarkdownTest::insertsPlaceholderWithoutSelection()
{
    const Edit edit = apply(Action::Italic, QStringLiteral("Learn "), 6, 0);
    QCOMPARE(edit.text, QStringLiteral("Learn _italic text_"));
    QCOMPARE(edit.text.mid(edit.selectionStart, edit.selectionLength), QStringLiteral("italic text"));
}

void LearningNotesMarkdownTest::prefixesEverySelectedLine()
{
    const Edit edit = apply(Action::Blockquote, QStringLiteral("one\ntwo"), 0, 7);
    QCOMPARE(edit.text, QStringLiteral("> one\n> two"));
}

void LearningNotesMarkdownTest::createsNumberedList()
{
    const Edit edit = apply(Action::NumberedList, QStringLiteral("one\ntwo"), 0, 7);
    QCOMPARE(edit.text, QStringLiteral("1. one\n2. two"));
}

void LearningNotesMarkdownTest::createsCodeBlockAndLink()
{
    QCOMPARE(apply(Action::CodeBlock, QStringLiteral("const n = 1;"), 0, 12).text,
             QStringLiteral("```\nconst n = 1;\n```"));
    QCOMPARE(apply(Action::Link, QStringLiteral("Qt docs"), 0, 7).text,
             QStringLiteral("[Qt docs](https://)"));
}

QTEST_MAIN(LearningNotesMarkdownTest)
#include "learningnotesmarkdown_test.moc"
