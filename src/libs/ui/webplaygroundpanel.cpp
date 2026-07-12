// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webplaygroundpanel.h"

#include "webplaygroundeditor.h"

#include <core/settings.h>

#include <QCheckBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

WebPlaygroundPanel::WebPlaygroundPanel(Core::Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_settings(settings)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *editorControls = new QHBoxLayout();
    m_editorTabs = new QTabBar(this);
    m_editorTabs->addTab(tr("HTML"));
    m_editorTabs->addTab(tr("CSS"));
    m_editorTabs->addTab(tr("JavaScript"));
    editorControls->addWidget(m_editorTabs);
    editorControls->addStretch();

    m_runButton = new QPushButton(tr("Run"), this);
    m_runButton->setEnabled(false);
    editorControls->addWidget(m_runButton);

    m_autoRun = new QCheckBox(tr("Auto Run"), this);
    m_autoRun->setEnabled(false);
    editorControls->addWidget(m_autoRun);
    layout->addLayout(editorControls);

    auto *editorContainer = new QWidget(this);
    m_editorLayout = new QVBoxLayout(editorContainer);
    m_editorLayout->setContentsMargins(0, 0, 0, 0);
    m_editorPlaceholder = new QLabel(tr("Code editor loads when the playground opens."), editorContainer);
    m_editorPlaceholder->setAlignment(Qt::AlignCenter);
    m_editorPlaceholder->setFrameShape(QFrame::StyledPanel);
    m_editorLayout->addWidget(m_editorPlaceholder);
    layout->addWidget(editorContainer, 1);

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
    for (const QString &text : {tr("Stop"), tr("Clear Console"), tr("Open in Browser"), tr("Export Project")}) {
        auto *button = new QPushButton(text, this);
        button->setEnabled(false);
        commands->addWidget(button);
    }
    m_resetButton = new QPushButton(tr("Reset"), this);
    m_resetButton->setEnabled(false);
    commands->insertWidget(1, m_resetButton);
    commands->addStretch();

    auto *closeButton = new QPushButton(tr("Close Playground"), this);
    connect(closeButton, &QPushButton::clicked, this, &WebPlaygroundPanel::closeRequested);
    commands->addWidget(closeButton);
    layout->addLayout(commands);

    connect(m_settings, &Core::Settings::updated, this, &WebPlaygroundPanel::applyAppearance);
}

void WebPlaygroundPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureInitialized();
}

void WebPlaygroundPanel::ensureInitialized()
{
    if (m_editor != nullptr) {
        return;
    }

    m_editor = new WebPlaygroundEditor(m_settings->isDarkModeEnabled(), this);
    m_editorLayout->replaceWidget(m_editorPlaceholder, m_editor);
    m_editorPlaceholder->deleteLater();
    m_editorPlaceholder = nullptr;

    connect(m_editorTabs, &QTabBar::currentChanged, m_editor, &WebPlaygroundEditor::setActiveEditor);
    connect(m_editor, &WebPlaygroundEditor::ready, this, [this]() {
        m_runButton->setEnabled(true);
        m_autoRun->setEnabled(true);
        m_resetButton->setEnabled(true);
        m_editor->setActiveEditor(m_editorTabs->currentIndex());
    });
}

void WebPlaygroundPanel::applyAppearance()
{
    if (m_editor != nullptr) {
        m_editor->setDark(m_settings->isDarkModeEnabled());
    }
}

} // namespace Zeal::WidgetUi
