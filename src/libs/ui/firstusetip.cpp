// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "firstusetip.h"

#include <core/settings.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace Zeal::WidgetUi {

FirstUseTip::FirstUseTip(Core::Settings *settings, const QString &id, const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_dismissed(settings->dismissedHelpTips.contains(id))
{
    setObjectName(QStringLiteral("firstUseTip"));
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 5, 5, 5);
    auto *label = new QLabel(text, this);
    label->setWordWrap(true);
    layout->addWidget(label, 1);
    auto *dismiss = new QPushButton(tr("Dismiss"), this);
    dismiss->setObjectName(QStringLiteral("dismissFirstUseTip"));
    dismiss->setToolTip(tr("Hide this tip"));
    layout->addWidget(dismiss);
    connect(dismiss, &QPushButton::clicked, this, [this, settings, id]() {
        if (!settings->dismissedHelpTips.contains(id)) {
            settings->dismissedHelpTips.append(id);
            settings->save();
        }
        m_dismissed = true;
        hide();
    });
    setVisible(!m_dismissed);
}

bool FirstUseTip::isDismissed() const
{
    return m_dismissed;
}

} // namespace Zeal::WidgetUi
