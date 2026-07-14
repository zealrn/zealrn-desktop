// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_QUICKTOURDIALOG_H
#define ZEAL_WIDGETUI_QUICKTOURDIALOG_H

#include <QWizard>

class QCheckBox;

namespace Zeal::WidgetUi {

class QuickTourDialog final : public QWizard
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(QuickTourDialog)

public:
    explicit QuickTourDialog(QWidget *parent = nullptr);

    bool doNotShowAutomatically() const;

signals:
    void openDocsetLibraryRequested();
    void openStartNoteRequested();

private:
    QCheckBox *m_doNotShowAgain = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_QUICKTOURDIALOG_H
