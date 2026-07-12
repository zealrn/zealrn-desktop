// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H

#include <QWidget>

namespace Zeal::WidgetUi {

class WebPlaygroundPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundPanel)
public:
    explicit WebPlaygroundPanel(QWidget *parent = nullptr);
    ~WebPlaygroundPanel() override = default;

signals:
    void closeRequested();
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
