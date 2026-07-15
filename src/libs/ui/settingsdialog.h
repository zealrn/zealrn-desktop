// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_SETTINGSDIALOG_H
#define ZEAL_WIDGETUI_SETTINGSDIALOG_H

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

namespace Zeal::WidgetUi {

namespace Ui {
class SettingsDialog;
} // namespace Ui

class SettingsDialog : public QDialog
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SettingsDialog)
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

    void chooseCustomCssFile();
    void chooseDocsetStoragePath();

private:
    void loadSettings();
    void saveSettings();

    Ui::SettingsDialog *ui = nullptr;
    QCheckBox *m_quickTourNextLaunchCheckBox = nullptr;
    QCheckBox *m_openStartNoteCheckBox = nullptr;
    QCheckBox *m_openLastDocumentationCheckBox = nullptr;
    QCheckBox *m_terminalStartOnOpenCheckBox = nullptr;
    QComboBox *m_updateFrequencyComboBox = nullptr;
    QCheckBox *m_updatePrereleasesCheckBox = nullptr;
    QLabel *m_updateLastCheckLabel = nullptr;
    QLabel *m_updateChannelLabel = nullptr;
    QPushButton *m_checkNowButton = nullptr;

    friend class Ui::SettingsDialog;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_SETTINGSDIALOG_H
