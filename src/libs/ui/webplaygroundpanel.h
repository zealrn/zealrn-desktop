// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;
class QShowEvent;
class QTabBar;
class QVBoxLayout;

namespace Zeal::Core {
class Settings;
}

namespace Zeal::WidgetUi {

class WebPlaygroundEditor;

class WebPlaygroundPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(WebPlaygroundPanel)
public:
    explicit WebPlaygroundPanel(Core::Settings *settings, QWidget *parent = nullptr);
    ~WebPlaygroundPanel() override = default;

signals:
    void closeRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void ensureInitialized();
    void applyAppearance();

    Core::Settings *m_settings = nullptr;
    QTabBar *m_editorTabs = nullptr;
    QVBoxLayout *m_editorLayout = nullptr;
    QLabel *m_editorPlaceholder = nullptr;
    QPushButton *m_runButton = nullptr;
    QCheckBox *m_autoRun = nullptr;
    QPushButton *m_resetButton = nullptr;
    WebPlaygroundEditor *m_editor = nullptr;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
