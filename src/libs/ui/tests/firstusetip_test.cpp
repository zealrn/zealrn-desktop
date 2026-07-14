// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../firstusetip.h"

#include <core/settings.h>

#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QtTest>

using Zeal::Core::Settings;
using Zeal::WidgetUi::FirstUseTip;

class FirstUseTipTest : public QObject
{
    Q_OBJECT

private slots:
    void dismissesAndPersistsTip();
};

void FirstUseTipTest::dismissesAndPersistsTip()
{
    QTemporaryDir directory;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
    QCoreApplication::setOrganizationName(QStringLiteral("FirstUseTipTest"));
    QCoreApplication::setApplicationName(QStringLiteral("FirstUseTipTest"));
    Settings settings;

    FirstUseTip tip(&settings, QStringLiteral("notes"), QStringLiteral("Helpful text"));
    QVERIFY(!tip.isDismissed());
    auto *dismiss = tip.findChild<QPushButton *>(QStringLiteral("dismissFirstUseTip"));
    QVERIFY(dismiss);
    dismiss->click();
    QVERIFY(settings.dismissedHelpTips.contains(QStringLiteral("notes")));

    FirstUseTip restored(&settings, QStringLiteral("notes"), QStringLiteral("Helpful text"));
    QVERIFY(restored.isDismissed());
}

QTEST_MAIN(FirstUseTipTest)
#include "firstusetip_test.moc"
