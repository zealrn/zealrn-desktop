// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotespanel.h"

#include <QCoreApplication>
#include <QFont>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

LearningNotesPanel::LearningNotesPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(QCoreApplication::translate("LearningNotesPanel", "Learning Notes"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    auto *status = new QLabel(QCoreApplication::translate("LearningNotesPanel", "Unsaved draft"), this);
    layout->addWidget(status);

    auto *editor = new QPlainTextEdit(this);
    editor->setPlaceholderText(
        QCoreApplication::translate("LearningNotesPanel", "Write what you learned from this documentation page…"));
    layout->addWidget(editor, 1);

    auto *clearDraftButton = new QPushButton(QCoreApplication::translate("LearningNotesPanel", "Clear Draft"), this);
    connect(clearDraftButton, &QPushButton::clicked, editor, &QPlainTextEdit::clear);
    layout->addWidget(clearDraftButton, 0, Qt::AlignRight);
}

} // namespace Zeal::WidgetUi
