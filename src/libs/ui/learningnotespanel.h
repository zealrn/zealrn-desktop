// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
#define ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H

#include <QWidget>

namespace Zeal::WidgetUi {

class LearningNotesPanel final : public QWidget
{
    Q_DISABLE_COPY_MOVE(LearningNotesPanel)
public:
    explicit LearningNotesPanel(QWidget *parent = nullptr);
    ~LearningNotesPanel() override = default;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_LEARNINGNOTESPANEL_H
