// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_FIRSTUSETIP_H
#define ZEAL_WIDGETUI_FIRSTUSETIP_H

#include <QWidget>

namespace Zeal::Core {
class Settings;
}

namespace Zeal::WidgetUi {

class FirstUseTip final : public QWidget
{
public:
    FirstUseTip(Core::Settings *settings, const QString &id, const QString &text, QWidget *parent = nullptr);

    bool isDismissed() const;

private:
    bool m_dismissed = false;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_FIRSTUSETIP_H
