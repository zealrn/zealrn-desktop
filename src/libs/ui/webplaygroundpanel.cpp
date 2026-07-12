// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundpanel.h"

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

WebPlaygroundPanel::WebPlaygroundPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *editorControls = new QHBoxLayout();
    auto *editorTabs = new QTabBar(this);
    editorTabs->addTab(tr("HTML"));
    editorTabs->addTab(tr("CSS"));
    editorTabs->addTab(tr("JavaScript"));
    editorControls->addWidget(editorTabs);
    editorControls->addStretch();

    auto *runButton = new QPushButton(tr("Run"), this);
    runButton->setEnabled(false);
    editorControls->addWidget(runButton);

    auto *autoRun = new QCheckBox(tr("Auto Run"), this);
    autoRun->setEnabled(false);
    editorControls->addWidget(autoRun);
    layout->addLayout(editorControls);

    auto *editorPlaceholder = new QLabel(tr("Code editor will load when editing is implemented."), this);
    editorPlaceholder->setAlignment(Qt::AlignCenter);
    editorPlaceholder->setFrameShape(QFrame::StyledPanel);
    layout->addWidget(editorPlaceholder, 1);

    auto *outputTabs = new QTabWidget(this);
    auto *previewPlaceholder = new QLabel(tr("Preview is not available yet."), outputTabs);
    previewPlaceholder->setAlignment(Qt::AlignCenter);
    outputTabs->addTab(previewPlaceholder, tr("Preview"));
    auto *consolePlaceholder = new QLabel(tr("Console output is not available yet."), outputTabs);
    consolePlaceholder->setAlignment(Qt::AlignCenter);
    outputTabs->addTab(consolePlaceholder, tr("Console"));
    layout->addWidget(outputTabs, 1);

    auto *warning = new QLabel(tr("Code runs locally inside an isolated preview."), this);
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto *commands = new QHBoxLayout();
    for (const QString &text : {tr("Stop"), tr("Reset"), tr("Clear Console"), tr("Open in Browser"),
                                tr("Export Project")}) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        commands->addWidget(button);
    }
    commands->addStretch();

    auto *closeButton = new QPushButton(tr("Close Playground"), this);
    connect(closeButton, &QPushButton::clicked, this, &WebPlaygroundPanel::closeRequested);
    commands->addWidget(closeButton);
    layout->addLayout(commands);
}

} // namespace Zeal::WidgetUi
