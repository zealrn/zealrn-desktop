// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "learningnotespanel.h"

#include "allnotesdialog.h"
#include "learningnotesmarkdown.h"
#include "learningnotesstore.h"
#include "widgets/iconhelper.h"

#include <core/settings.h>

#include <QAction>
#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShortcut>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QVariant>

namespace {

QString exportDirectory()
{
    QString directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    return directory.isEmpty() ? QDir::homePath() : directory;
}

bool confirmOverwrite(QWidget *parent, const QString &path)
{
    return !QFileInfo::exists(path)
        || QMessageBox::question(parent,
                                 QCoreApplication::translate("LearningNotesPanel", "Replace File"),
                                 QCoreApplication::translate("LearningNotesPanel", "Replace the existing file?"))
               == QMessageBox::Yes;
}

class MarkdownPreview final : public QTextBrowser
{
public:
    using QTextBrowser::QTextBrowser;

protected:
    QVariant loadResource(int, const QUrl &) override
    {
        return {};
    }
};

} // namespace

namespace Zeal::WidgetUi {

LearningNotesPanel::LearningNotesPanel(QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>())
{
    setupUi();
}

LearningNotesPanel::LearningNotesPanel(Core::Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>())
    , m_settings(settings)
{
    setupUi();
}

LearningNotesPanel::LearningNotesPanel(const QString &databasePath, QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>(databasePath))
{
    setupUi();
}

LearningNotesPanel::LearningNotesPanel(const QString &databasePath, Core::Settings *settings, QWidget *parent)
    : QWidget(parent)
    , m_store(std::make_unique<LearningNotesStore>(databasePath))
    , m_settings(settings)
{
    setupUi();
}

LearningNotesPanel::~LearningNotesPanel()
{
    flush();
}

void LearningNotesPanel::setupUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(6);

    auto *titleLayout = new QHBoxLayout();
    auto *title = new QLabel(QCoreApplication::translate("LearningNotesPanel", "Learning Notes"), this);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    auto *startNoteButton = new QToolButton(this);
    startNoteButton->setObjectName(QStringLiteral("startNoteButton"));
    startNoteButton->setText(tr("Start Note"));
    startNoteButton->setToolTip(tr("Open the note that is always available"));
    titleLayout->addWidget(startNoteButton);

    auto *expandAction = new QAction(tr("Expand Notes"), this);
    m_expandAction = expandAction;
    expandAction->setObjectName(QStringLiteral("noteExpandAction"));
    expandAction->setCheckable(true);
    expandAction->setToolTip(tr("Give Learning Notes more width"));
    auto *expandButton = new QToolButton(this);
    expandButton->setDefaultAction(expandAction);
    expandButton->setAutoRaise(true);
    titleLayout->addWidget(expandButton);

    auto *focusAction = new QAction(tr("Focus Notes"), this);
    m_focusAction = focusAction;
    focusAction->setObjectName(QStringLiteral("noteFocusAction"));
    focusAction->setCheckable(true);
    focusAction->setToolTip(tr("Hide the sidebar and development tools temporarily"));
    auto *focusButton = new QToolButton(this);
    focusButton->setDefaultAction(focusAction);
    focusButton->setAutoRaise(true);
    titleLayout->addWidget(focusButton);
    layout->addLayout(titleLayout);

    m_docsetLabel = new QLabel(this);
    m_docsetLabel->setWordWrap(true);
    layout->addWidget(m_docsetLabel);

    m_pageLabel = new QLabel(this);
    m_pageLabel->setWordWrap(true);
    layout->addWidget(m_pageLabel);

    m_pathLabel = new QLabel(this);
    m_pathLabel->setWordWrap(true);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(m_pathLabel);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("noteStatus"));
    m_statusLabel->setToolTip(tr("Notes save automatically after one second."));
    m_savedAtLabel = new QLabel(this);
    auto *statusLayout = new QHBoxLayout();
    statusLayout->addWidget(m_statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_savedAtLabel);
    layout->addLayout(statusLayout);

    auto *formatBar = new QToolBar(this);
    formatBar->setObjectName(QStringLiteral("noteFormatBar"));
    formatBar->setMovable(false);
    formatBar->setFloatable(false);
    formatBar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    const auto addFormatAction = [this, formatBar](const QString &text,
                                                   const QString &name,
                                                   const QString &tooltip,
                                                   LearningNotesMarkdown::Action format) {
        QAction *action = formatBar->addAction(text);
        action->setObjectName(name);
        action->setToolTip(tooltip);
        connect(action, &QAction::triggered, this, [this, format]() { applyFormat(static_cast<int>(format)); });
        return action;
    };
    addFormatAction(QStringLiteral("H"), QStringLiteral("noteHeadingAction"), tr("Heading"),
                    LearningNotesMarkdown::Action::Heading);
    QAction *boldAction = addFormatAction(QStringLiteral("B"), QStringLiteral("noteBoldAction"), tr("Bold"),
                                          LearningNotesMarkdown::Action::Bold);
    QFont boldFont = boldAction->font();
    boldFont.setBold(true);
    boldAction->setFont(boldFont);
    QAction *italicAction = addFormatAction(QStringLiteral("I"), QStringLiteral("noteItalicAction"), tr("Italic"),
                                            LearningNotesMarkdown::Action::Italic);
    QFont italicFont = italicAction->font();
    italicFont.setItalic(true);
    italicAction->setFont(italicFont);
    addFormatAction(QStringLiteral("-"), QStringLiteral("noteBulletAction"), tr("Bullet list"),
                    LearningNotesMarkdown::Action::BulletList);
    addFormatAction(QStringLiteral("1."), QStringLiteral("noteNumberedAction"), tr("Numbered list"),
                    LearningNotesMarkdown::Action::NumberedList);
    addFormatAction(QStringLiteral("[ ]"), QStringLiteral("noteTaskAction"), tr("Task checkbox"),
                    LearningNotesMarkdown::Action::Task);
    addFormatAction(QStringLiteral(">"), QStringLiteral("noteQuoteAction"), tr("Blockquote"),
                    LearningNotesMarkdown::Action::Blockquote);
    addFormatAction(QStringLiteral("`"), QStringLiteral("noteInlineCodeAction"), tr("Inline code"),
                    LearningNotesMarkdown::Action::InlineCode);
    addFormatAction(QStringLiteral("```"), QStringLiteral("noteCodeBlockAction"), tr("Code block"),
                    LearningNotesMarkdown::Action::CodeBlock);
    addFormatAction(tr("Link"), QStringLiteral("noteLinkAction"), tr("Link"),
                    LearningNotesMarkdown::Action::Link);
    addFormatAction(QStringLiteral("---"), QStringLiteral("noteRuleAction"), tr("Horizontal separator"),
                    LearningNotesMarkdown::Action::HorizontalRule);
    formatBar->findChild<QAction *>(QStringLiteral("noteNumberedAction"))
        ->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+7")));
    formatBar->findChild<QAction *>(QStringLiteral("noteBulletAction"))
        ->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+8")));
    formatBar->findChild<QAction *>(QStringLiteral("noteQuoteAction"))
        ->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+9")));
    formatBar->addSeparator();
    QAction *undoAction = formatBar->addAction(
        IconHelper::fromTheme(QStringLiteral("edit-undo"), QStringLiteral(":/icons/tabler/arrow-back-up.svg")),
        tr("Undo"));
    undoAction->setObjectName(QStringLiteral("noteUndoAction"));
    QAction *redoAction = formatBar->addAction(
        IconHelper::fromTheme(QStringLiteral("edit-redo"), QStringLiteral(":/icons/tabler/arrow-forward-up.svg")),
        tr("Redo"));
    redoAction->setObjectName(QStringLiteral("noteRedoAction"));
    QAction *lineWrapAction = formatBar->addAction(tr("Wrap"));
    lineWrapAction->setObjectName(QStringLiteral("noteLineWrapAction"));
    lineWrapAction->setCheckable(true);
    lineWrapAction->setChecked(m_settings == nullptr || m_settings->learningNotesLineWrap);
    layout->addWidget(formatBar);

    auto *findLayout = new QHBoxLayout();
    auto *findWidget = new QWidget(this);
    findWidget->setObjectName(QStringLiteral("noteFindBar"));
    findWidget->setLayout(findLayout);
    m_findEdit = new QLineEdit(findWidget);
    m_findEdit->setObjectName(QStringLiteral("noteFindEdit"));
    m_findEdit->setPlaceholderText(tr("Find in note"));
    findLayout->addWidget(m_findEdit, 1);
    QAction *findPreviousAction = new QAction(tr("Previous"), this);
    findPreviousAction->setObjectName(QStringLiteral("noteFindPreviousAction"));
    auto *findPreviousButton = new QToolButton(findWidget);
    findPreviousButton->setDefaultAction(findPreviousAction);
    findLayout->addWidget(findPreviousButton);
    QAction *findNextAction = new QAction(tr("Next"), this);
    findNextAction->setObjectName(QStringLiteral("noteFindNextAction"));
    auto *findNextButton = new QToolButton(findWidget);
    findNextButton->setDefaultAction(findNextAction);
    findLayout->addWidget(findNextButton);
    auto *closeFindButton = new QToolButton(findWidget);
    closeFindButton->setText(QStringLiteral("×"));
    closeFindButton->setToolTip(tr("Close find bar"));
    findLayout->addWidget(closeFindButton);
    findWidget->hide();
    layout->addWidget(findWidget);

    m_editor = new QPlainTextEdit(this);
    m_editor->setObjectName(QStringLiteral("noteEditor"));
    m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_editor->setPlaceholderText(
        QCoreApplication::translate("LearningNotesPanel", "Write what you learned from this documentation page…"));
    m_preview = new MarkdownPreview(this);
    m_preview->setObjectName(QStringLiteral("notePreview"));
    m_preview->setOpenLinks(false);
    m_preview->setOpenExternalLinks(false);
    m_modeTabs = new QTabWidget(this);
    m_modeTabs->setObjectName(QStringLiteral("noteModeTabs"));
    m_modeTabs->addTab(m_editor, tr("Edit"));
    m_modeTabs->addTab(m_preview, tr("Preview"));
    layout->addWidget(m_modeTabs, 1);

    auto *detailLayout = new QHBoxLayout();
    m_countLabel = new QLabel(this);
    m_countLabel->setObjectName(QStringLiteral("noteCount"));
    detailLayout->addWidget(m_countLabel);
    detailLayout->addStretch();
    QAction *zoomOutAction = new QAction(
        IconHelper::fromTheme(QStringLiteral("zoom-out"), QStringLiteral(":/icons/tabler/minus.svg")),
        tr("Zoom Out"),
        this);
    zoomOutAction->setObjectName(QStringLiteral("noteZoomOutAction"));
    auto *zoomOutButton = new QToolButton(this);
    zoomOutButton->setDefaultAction(zoomOutAction);
    zoomOutButton->setAutoRaise(true);
    detailLayout->addWidget(zoomOutButton);
    m_zoomLabel = new QLabel(this);
    m_zoomLabel->setObjectName(QStringLiteral("noteZoom"));
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(44);
    detailLayout->addWidget(m_zoomLabel);
    QAction *zoomInAction = new QAction(
        IconHelper::fromTheme(QStringLiteral("zoom-in"), QStringLiteral(":/icons/tabler/plus.svg")),
        tr("Zoom In"),
        this);
    zoomInAction->setObjectName(QStringLiteral("noteZoomInAction"));
    auto *zoomInButton = new QToolButton(this);
    zoomInButton->setDefaultAction(zoomInAction);
    zoomInButton->setAutoRaise(true);
    detailLayout->addWidget(zoomInButton);
    QAction *zoomResetAction = new QAction(
        IconHelper::fromTheme(QStringLiteral("zoom-original"), QStringLiteral(":/icons/tabler/zoom-reset.svg")),
        tr("Reset Zoom"),
        this);
    zoomResetAction->setObjectName(QStringLiteral("noteZoomResetAction"));
    auto *zoomResetButton = new QToolButton(this);
    zoomResetButton->setDefaultAction(zoomResetAction);
    zoomResetButton->setAutoRaise(true);
    detailLayout->addWidget(zoomResetButton);
    layout->addLayout(detailLayout);

    auto *buttonLayout = new QGridLayout();
    m_saveButton = new QPushButton(QCoreApplication::translate("LearningNotesPanel", "Save Note"), this);
    buttonLayout->addWidget(m_saveButton, 0, 0);

    m_addSelectionButton = new QPushButton(
        QCoreApplication::translate("LearningNotesPanel", "Add Selection"), this);
    buttonLayout->addWidget(m_addSelectionButton, 0, 1);

    m_clearButton = new QPushButton(tr("Clear"), this);
    buttonLayout->addWidget(m_clearButton, 0, 2);

    auto *allNotesButton = new QPushButton(QCoreApplication::translate("LearningNotesPanel", "All Notes"), this);
    buttonLayout->addWidget(allNotesButton, 1, 0);

    m_exportButton = new QToolButton(this);
    m_exportButton->setText(QCoreApplication::translate("LearningNotesPanel", "Export"));
    m_exportButton->setPopupMode(QToolButton::InstantPopup);
    auto *exportMenu = new QMenu(m_exportButton);
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export Markdown"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Markdown);
    });
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export PDF"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Pdf);
    });
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export JSON"), this, [this]() {
        exportNote(m_note, LearningNotesExport::Format::Json);
    });
    exportMenu->addSeparator();
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Export All Notes"),
                          this,
                          &LearningNotesPanel::exportAllNotes);
    exportMenu->addAction(QCoreApplication::translate("LearningNotesPanel", "Backup Database"),
                          this,
                          &LearningNotesPanel::backupDatabase);
    m_exportButton->setMenu(exportMenu);
    buttonLayout->addWidget(m_exportButton, 1, 1);
    layout->addLayout(buttonLayout);

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setSingleShot(true);
    m_autoSaveTimer->setInterval(1000);
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(200);

    connect(m_editor, &QPlainTextEdit::textChanged, this, [this]() {
        updateCounts();
        m_previewTimer->start();
        if (m_loading || !m_note.page.isValid()) {
            return;
        }
        m_dirty = true;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Unsaved"));
        m_autoSaveTimer->start();
    });
    connect(m_editor, &QPlainTextEdit::selectionChanged, this, &LearningNotesPanel::updateCounts);
    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() { save(false); });
    connect(m_saveButton, &QPushButton::clicked, this, [this]() { save(true); });
    connect(m_clearButton, &QPushButton::clicked, this, &LearningNotesPanel::clearCurrentNote);
    connect(startNoteButton, &QToolButton::clicked, this, [this]() { showStartNote(); });
    connect(m_addSelectionButton, &QPushButton::clicked, this, &LearningNotesPanel::addSelectionRequested);
    connect(allNotesButton, &QPushButton::clicked, this, &LearningNotesPanel::showAllNotes);
    connect(m_previewTimer, &QTimer::timeout, this, &LearningNotesPanel::updatePreview);
    connect(m_modeTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1) {
            updatePreview();
        }
    });
    connect(m_preview, &QTextBrowser::anchorClicked, this, [this](const QUrl &url) {
        const QString scheme = url.scheme().toLower();
        if (!url.isValid()
            || (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")
                && scheme != QStringLiteral("mailto"))) {
            QMessageBox::warning(this, tr("Link Blocked"), tr("This link type cannot be opened from note preview."));
            return;
        }
        if (QMessageBox::question(this,
                                  tr("Open External Link"),
                                  tr("Open this link outside ZealRN?\n\n%1").arg(url.toDisplayString()))
            == QMessageBox::Yes) {
            QDesktopServices::openUrl(url);
        }
    });
    connect(undoAction, &QAction::triggered, m_editor, &QPlainTextEdit::undo);
    connect(redoAction, &QAction::triggered, m_editor, &QPlainTextEdit::redo);
    connect(lineWrapAction, &QAction::toggled, this, &LearningNotesPanel::setLineWrap);
    connect(findPreviousAction, &QAction::triggered, this, [this]() { findNote(true); });
    connect(findNextAction, &QAction::triggered, this, [this]() { findNote(false); });
    connect(m_findEdit, &QLineEdit::returnPressed, this, [this]() { findNote(false); });
    connect(closeFindButton, &QToolButton::clicked, findWidget, &QWidget::hide);
    connect(zoomOutAction, &QAction::triggered, this, [this]() {
        setZoom((m_settings ? m_settings->learningNotesZoom : 115) - 10);
    });
    connect(zoomInAction, &QAction::triggered, this, [this]() {
        setZoom((m_settings ? m_settings->learningNotesZoom : 115) + 10);
    });
    connect(zoomResetAction, &QAction::triggered, this, [this]() { setZoom(115); });
    connect(expandAction, &QAction::toggled, this, &LearningNotesPanel::expandedModeRequested);
    connect(focusAction, &QAction::toggled, this, &LearningNotesPanel::focusModeRequested);
    connect(expandAction, &QAction::toggled, this, [expandAction](bool expanded) {
        expandAction->setText(expanded ? tr("Restore Notes") : tr("Expand Notes"));
    });
    connect(focusAction, &QAction::toggled, this, [focusAction](bool focused) {
        focusAction->setText(focused ? tr("Exit Focus") : tr("Focus Notes"));
    });
    connect(focusAction, &QAction::toggled, expandAction, [expandAction](bool focused) {
        expandAction->setEnabled(!focused);
    });
    auto *saveShortcut = new QShortcut(QKeySequence::Save, this);
    connect(saveShortcut, &QShortcut::activated, this, [this]() { save(true); });
    auto *findAction = new QAction(this);
    findAction->setObjectName(QStringLiteral("noteFindAction"));
    findAction->setShortcut(QKeySequence::Find);
    findAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addAction(findAction);
    connect(findAction, &QAction::triggered, this, [findWidget, this]() {
        findWidget->show();
        m_findEdit->selectAll();
        m_findEdit->setFocus();
    });
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    zoomOutAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    zoomInAction->setShortcuts({QKeySequence::ZoomIn, QKeySequence(QStringLiteral("Ctrl+="))});
    zoomInAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    zoomResetAction->setShortcut(QKeySequence(QStringLiteral("Ctrl+0")));
    zoomResetAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    addActions({zoomOutAction, zoomInAction, zoomResetAction});

    setLineWrap(lineWrapAction->isChecked());
    setZoom(m_settings ? m_settings->learningNotesZoom : 115);
    updateCounts();

    showStartNote();
}

void LearningNotesPanel::applyFormat(int action)
{
    const QTextCursor current = m_editor->textCursor();
    const QString before = m_editor->toPlainText();
    const auto edit = LearningNotesMarkdown::apply(static_cast<LearningNotesMarkdown::Action>(action),
                                                   before,
                                                   current.selectionStart(),
                                                   current.selectionEnd() - current.selectionStart());
    qsizetype prefix = 0;
    while (prefix < before.size() && prefix < edit.text.size() && before.at(prefix) == edit.text.at(prefix)) {
        ++prefix;
    }
    qsizetype suffix = 0;
    while (suffix < before.size() - prefix && suffix < edit.text.size() - prefix
           && before.at(before.size() - suffix - 1) == edit.text.at(edit.text.size() - suffix - 1)) {
        ++suffix;
    }

    QTextCursor cursor(m_editor->document());
    cursor.beginEditBlock();
    cursor.setPosition(static_cast<int>(prefix));
    cursor.setPosition(static_cast<int>(before.size() - suffix), QTextCursor::KeepAnchor);
    cursor.insertText(edit.text.mid(prefix, edit.text.size() - prefix - suffix));
    cursor.endEditBlock();
    cursor.setPosition(edit.selectionStart);
    cursor.setPosition(edit.selectionStart + edit.selectionLength, QTextCursor::KeepAnchor);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus();
}

void LearningNotesPanel::updatePreview()
{
    QTextDocument::MarkdownFeatures features(QTextDocument::MarkdownDialectGitHub);
    features.setFlag(QTextDocument::MarkdownNoHTML);
    m_preview->document()->setMarkdown(m_editor->toPlainText(), features);
}

void LearningNotesPanel::exitFocusMode()
{
    m_focusAction->setChecked(false);
}

void LearningNotesPanel::exitExpandedMode()
{
    m_expandAction->setChecked(false);
}

void LearningNotesPanel::updateCounts()
{
    const QTextCursor cursor = m_editor->textCursor();
    const auto value = LearningNotesMarkdown::counts(m_editor->toPlainText(),
                                                     cursor.selectionStart(),
                                                     cursor.selectionEnd() - cursor.selectionStart());
    QString text = tr("%1 words · %2 characters").arg(value.words).arg(value.characters);
    if (value.selectedCharacters > 0) {
        text += tr(" · %1 selected").arg(value.selectedCharacters);
    }
    m_countLabel->setText(text);
}

void LearningNotesPanel::findNote(bool backward)
{
    if (m_findEdit->text().isEmpty()) {
        return;
    }
    QTextDocument::FindFlags flags;
    flags.setFlag(QTextDocument::FindBackward, backward);
    if (m_editor->find(m_findEdit->text(), flags)) {
        return;
    }
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    m_editor->setTextCursor(cursor);
    m_editor->find(m_findEdit->text(), flags);
}

void LearningNotesPanel::setZoom(int percent)
{
    const int zoom = LearningNotesMarkdown::clampZoom(percent);
    QFont editorFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    editorFont.setPointSizeF(editorFont.pointSizeF() * zoom / 100.0);
    m_editor->setFont(editorFont);
    QFont previewFont = font();
    previewFont.setPointSizeF(previewFont.pointSizeF() * zoom / 100.0);
    m_preview->setFont(previewFont);
    m_zoomLabel->setText(tr("%1%").arg(zoom));
    if (m_settings != nullptr && m_settings->learningNotesZoom != zoom) {
        m_settings->learningNotesZoom = zoom;
        m_settings->save();
    }
}

void LearningNotesPanel::setLineWrap(bool enabled)
{
    m_editor->setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
    if (m_settings != nullptr && m_settings->learningNotesLineWrap != enabled) {
        m_settings->learningNotesLineWrap = enabled;
        m_settings->save();
    }
}

bool LearningNotesPanel::setPage(const LearningNotePage &page)
{
    if (m_note.page.docsetId == page.docsetId && m_note.page.pageKey == page.pageKey && page.isValid()) {
        m_note.page = page;
        m_docsetLabel->setText(page.docsetName);
        m_pageLabel->setText(page.pageTitle);
        m_pathLabel->setText(page.pagePath);
        return true;
    }
    if (!flush()) {
        return false;
    }

    m_autoSaveTimer->stop();
    m_previewTimer->stop();
    m_note = {};
    m_note.page = page;
    m_lastSelection.clear();
    m_loading = true;
    m_editor->clear();
    m_loading = false;
    m_dirty = false;

    const bool valid = page.isValid();
    const bool startNote = page.isStartNote();
    m_editor->setEnabled(valid);
    m_modeTabs->setEnabled(valid);
    m_saveButton->setEnabled(valid);
    m_addSelectionButton->setEnabled(valid && !startNote);
    m_clearButton->setEnabled(valid);
    m_exportButton->setEnabled(true);
    m_docsetLabel->setText(startNote ? tr("Always available") : valid ? page.docsetName : QString());
    m_pageLabel->setText(valid ? page.pageTitle : QCoreApplication::translate(
                                                      "LearningNotesPanel", "No documentation page selected"));
    m_pathLabel->setText(startNote ? tr("What are you learning today?") : valid ? page.pagePath : QString());
    m_editor->setPlaceholderText(startNote ? tr("What are you learning today?")
                                           : tr("Write what you learned from this documentation page…"));
    m_savedAtLabel->clear();
    if (!valid) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "No page"));
        return true;
    }

    if (const auto saved = m_store->note(page.docsetId, page.pageKey)) {
        m_note = *saved;
        m_loading = true;
        m_editor->setPlainText(m_note.content);
        m_loading = false;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Saved"));
        m_savedAtLabel->setText(m_note.updatedAt.toLocalTime().toString(QStringLiteral("HH:mm")));
    } else if (!m_store->lastError().isEmpty()) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Save failed"));
        m_editor->setToolTip(m_store->lastError());
    } else {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "New note"));
    }
    updatePreview();
    return true;
}

bool LearningNotesPanel::showStartNote()
{
    return setPage(LearningNotePage::startNote());
}

const LearningNotePage &LearningNotesPanel::currentPage() const
{
    return m_note.page;
}

bool LearningNotesPanel::flush()
{
    m_autoSaveTimer->stop();
    return !m_dirty || save(false);
}

void LearningNotesPanel::appendSelection(const QString &selection)
{
    const QString trimmed = selection.trimmed();
    if (!m_note.page.isValid() || trimmed.isEmpty() || trimmed == m_lastSelection) {
        return;
    }

    QString quote = trimmed;
    quote.replace(QLatin1Char('\n'), QStringLiteral("\n> "));
    const QString separator = m_editor->toPlainText().isEmpty() ? QString() : QStringLiteral("\n\n");
    m_editor->moveCursor(QTextCursor::End);
    m_editor->insertPlainText(separator + QStringLiteral("> ") + quote);
    m_lastSelection = trimmed;
}

bool LearningNotesPanel::save(bool explicitSave)
{
    if (!m_note.page.isValid()) {
        return false;
    }
    m_note.content = m_editor->toPlainText();
    if (!explicitSave && m_note.id == 0 && m_note.content.trimmed().isEmpty()) {
        m_dirty = false;
        setStatus(QCoreApplication::translate("LearningNotesPanel", "New note"));
        return true;
    }

    setStatus(QCoreApplication::translate("LearningNotesPanel", "Saving…"));
    if (!m_store->save(&m_note)) {
        setStatus(QCoreApplication::translate("LearningNotesPanel", "Save failed"));
        m_editor->setToolTip(m_store->lastError());
        return false;
    }
    m_dirty = false;
    m_editor->setToolTip({});
    setStatus(QCoreApplication::translate("LearningNotesPanel", "Saved"));
    m_savedAtLabel->setText(m_note.updatedAt.toLocalTime().toString(QStringLiteral("HH:mm")));
    emit noteSaved(m_note.page);
    return true;
}

void LearningNotesPanel::clearCurrentNote()
{
    if (m_editor->toPlainText().isEmpty()) {
        return;
    }
    if (QMessageBox::question(this, tr("Clear Note"), tr("Clear the current note?")) != QMessageBox::Yes) {
        return;
    }
    m_editor->clear();
}

void LearningNotesPanel::showAllNotes()
{
    if (!flush()) {
        return;
    }
    AllNotesDialog dialog(m_store.get(), this);
    connect(&dialog, &AllNotesDialog::openDocumentationRequested, this, &LearningNotesPanel::openDocumentationRequested);
    connect(&dialog, &AllNotesDialog::exportRequested, this, &LearningNotesPanel::exportNote);
    dialog.exec();

    const LearningNotePage page = m_note.page;
    m_note = {};
    setPage(page);
}

void LearningNotesPanel::exportNote(const LearningNote &note, LearningNotesExport::Format format)
{
    if (!note.page.isValid()) {
        return;
    }
    const bool isCurrent = note.page.docsetId == m_note.page.docsetId && note.page.pageKey == m_note.page.pageKey;
    if (isCurrent && !flush()) {
        return;
    }
    const LearningNote value = isCurrent ? m_note : note;
    const QString base = LearningNotesExport::safeFileName(value.page.docsetName, value.page.pageTitle);
    const QString extension = format == LearningNotesExport::Format::Markdown
                                ? QStringLiteral("md")
                                : format == LearningNotesExport::Format::Pdf ? QStringLiteral("pdf")
                                                                            : QStringLiteral("json");
    const QString path = QFileDialog::getSaveFileName(this,
                                                      tr("Export Learning Note"),
                                                      QDir(exportDirectory()).filePath(base + QLatin1Char('.')
                                                                                       + extension));
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }

    QString error;
    LearningNotesExport::Result result = LearningNotesExport::Result::Error;
    if (format == LearningNotesExport::Format::Markdown) {
        result = LearningNotesExport::writeMarkdown(path, value, true, &error);
    } else if (format == LearningNotesExport::Format::Pdf) {
        result = LearningNotesExport::writePdf(path, value, true, &error);
    } else {
        result = LearningNotesExport::writeJson(path, value, true, &error);
    }
    if (result == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Export Complete"), tr("The note was exported successfully."));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), error);
    }
}

void LearningNotesPanel::exportAllNotes()
{
    if (!flush()) {
        return;
    }
    const QList<LearningNote> notes = m_store->search();
    if (notes.isEmpty()) {
        QMessageBox::information(this, tr("Export All Notes"), tr("There are no saved notes to export."));
        return;
    }
    const QString suggested = QDir(exportDirectory())
                                  .filePath(QStringLiteral("notes-export-%1.zip").arg(
                                      QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    const QString path = QFileDialog::getSaveFileName(this, tr("Export All Notes"), suggested);
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }
    QString error;
    if (LearningNotesExport::writeAllZip(path, notes, true, &error) == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Export Complete"), tr("All notes were exported successfully."));
    } else {
        QMessageBox::warning(this, tr("Export Failed"), error);
    }
}

void LearningNotesPanel::backupDatabase()
{
    if (!flush() || !m_store->checkpoint()) {
        QMessageBox::warning(this, tr("Backup Failed"), m_store->lastError());
        return;
    }
    const QString suggested = QDir(exportDirectory()).filePath(QStringLiteral("learning-notes-backup.sqlite"));
    const QString path = QFileDialog::getSaveFileName(this, tr("Backup Notes Database"), suggested);
    if (path.isEmpty() || !confirmOverwrite(this, path)) {
        return;
    }
    QString error;
    if (LearningNotesExport::copyDatabase(m_store->databasePath(), path, true, &error)
        == LearningNotesExport::Result::Success) {
        QMessageBox::information(this, tr("Backup Complete"), tr("The notes database was backed up successfully."));
    } else {
        QMessageBox::warning(this, tr("Backup Failed"), error);
    }
}

void LearningNotesPanel::setStatus(const QString &status)
{
    m_statusLabel->setText(status);
}

} // namespace Zeal::WidgetUi
