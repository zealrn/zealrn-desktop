// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalbackend.h"

namespace Zeal::WidgetUi {

TerminalBackend::TerminalBackend(QObject *parent)
    : QObject(parent)
{
}

static const int TerminalProfileMetaType = qRegisterMetaType<TerminalProfile>();

} // namespace Zeal::WidgetUi
