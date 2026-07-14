// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESMARKDOWN_H
#define ZEAL_WIDGETUI_LEARNINGNOTESMARKDOWN_H

#include <QString>

namespace Zeal::WidgetUi::LearningNotesMarkdown {

enum class Action {
    Heading,
    Bold,
    Italic,
    BulletList,
    NumberedList,
    Task,
    Blockquote,
    InlineCode,
    CodeBlock,
    Link,
    HorizontalRule,
};

struct Edit
{
    QString text;
    int selectionStart = 0;
    int selectionLength = 0;
};

struct Counts
{
    int words = 0;
    int characters = 0;
    int selectedCharacters = 0;
};

int clampZoom(int percent);
Counts counts(const QString &text, int selectionStart = 0, int selectionLength = 0);
Edit apply(Action action, const QString &text, int selectionStart, int selectionLength);

} // namespace Zeal::WidgetUi::LearningNotesMarkdown

#endif // ZEAL_WIDGETUI_LEARNINGNOTESMARKDOWN_H
