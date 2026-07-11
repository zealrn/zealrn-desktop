// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotespanel.h"

#include <QCoreApplication>
#include <QFont>
#include <QLabel>
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

    auto *description = new QLabel(
        QCoreApplication::translate(
            "LearningNotesPanel",
            "Notes, highlights, snippets, and bookmarks saved from documentation pages will appear here."),
        this);
    description->setWordWrap(true);
    layout->addWidget(description);
    layout->addStretch();
}

} // namespace Zeal::WidgetUi
