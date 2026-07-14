// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotesmarkdown.h"

#include <QRegularExpression>
#include <QStringList>

namespace Zeal::WidgetUi::LearningNotesMarkdown {

namespace {

Edit replaceSelection(const QString &text,
                      int start,
                      int length,
                      const QString &replacement,
                      int innerOffset,
                      int innerLength)
{
    Edit edit{text};
    edit.text.replace(start, length, replacement);
    edit.selectionStart = start + innerOffset;
    edit.selectionLength = innerLength;
    return edit;
}

Edit wrap(const QString &text,
          int start,
          int length,
          const QString &before,
          const QString &after,
          const QString &placeholder)
{
    const QString inner = length > 0 ? text.mid(start, length) : placeholder;
    return replaceSelection(text, start, length, before + inner + after, before.size(), inner.size());
}

Edit prefixLines(const QString &text, int start, int length, Action action)
{
    QString selected = length > 0 ? text.mid(start, length) : QStringLiteral("list item");
    QStringList lines = selected.split(QLatin1Char('\n'));
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QString prefix;
        switch (action) {
        case Action::Heading:
            prefix = QStringLiteral("## ");
            break;
        case Action::BulletList:
            prefix = QStringLiteral("- ");
            break;
        case Action::NumberedList:
            prefix = QString::number(i + 1) + QStringLiteral(". ");
            break;
        case Action::Task:
            prefix = QStringLiteral("- [ ] ");
            break;
        case Action::Blockquote:
            prefix = QStringLiteral("> ");
            break;
        default:
            break;
        }
        lines[i].prepend(prefix);
    }
    const QString replacement = lines.join(QLatin1Char('\n'));
    return replaceSelection(text, start, length, replacement, 0, replacement.size());
}

} // namespace

int clampZoom(int percent)
{
    return qBound(80, percent, 200);
}

Counts counts(const QString &text, int selectionStart, int selectionLength)
{
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));
    const int safeStart = qBound(0, selectionStart, text.size());
    const int safeLength = qBound(0, selectionLength, text.size() - safeStart);
    return {.words = static_cast<int>(text.split(whitespace, Qt::SkipEmptyParts).size()),
            .characters = static_cast<int>(text.size()),
            .selectedCharacters = safeLength};
}

Edit apply(Action action, const QString &text, int selectionStart, int selectionLength)
{
    const int start = qBound(0, selectionStart, text.size());
    const int length = qBound(0, selectionLength, text.size() - start);
    switch (action) {
    case Action::Heading:
    case Action::BulletList:
    case Action::NumberedList:
    case Action::Task:
    case Action::Blockquote:
        return prefixLines(text, start, length, action);
    case Action::Bold:
        return wrap(text, start, length, QStringLiteral("**"), QStringLiteral("**"), QStringLiteral("bold text"));
    case Action::Italic:
        return wrap(text, start, length, QStringLiteral("_"), QStringLiteral("_"), QStringLiteral("italic text"));
    case Action::InlineCode:
        return wrap(text, start, length, QStringLiteral("`"), QStringLiteral("`"), QStringLiteral("code"));
    case Action::CodeBlock:
        return wrap(text, start, length, QStringLiteral("```\n"), QStringLiteral("\n```"), QStringLiteral("code"));
    case Action::Link:
        return wrap(text, start, length, QStringLiteral("["), QStringLiteral("](https://)"), QStringLiteral("link text"));
    case Action::HorizontalRule:
        return replaceSelection(text, start, length, QStringLiteral("\n---\n"), 5, 0);
    }
    return {text, start, length};
}

} // namespace Zeal::WidgetUi::LearningNotesMarkdown
