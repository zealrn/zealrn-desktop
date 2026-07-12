// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
#define ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H

#include <QWidget>
#include <QElapsedTimer>

class QCheckBox;
class QLabel;
class QListWidget;
class QPushButton;
class QHideEvent;
class QShowEvent;
class QTabBar;
class QTabWidget;
class QTimer;
class QVBoxLayout;

namespace Zeal::Core {
class Settings;
}

namespace Zeal::WidgetUi {

class WebPlaygroundEditor;
class WebPlaygroundPreview;

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
    void hideEvent(QHideEvent *event) override;

private:
    void ensureInitialized();
    void applyAppearance();
    void runPreview();
    void stopPreview();
    void resetDocuments();
    void appendConsole(const QString &severity,
                       const QString &message,
                       int lineNumber = 0,
                       const QString &sourceId = {});
    void clearConsole();

    Core::Settings *m_settings = nullptr;
    QTabBar *m_editorTabs = nullptr;
    QVBoxLayout *m_editorLayout = nullptr;
    QLabel *m_editorPlaceholder = nullptr;
    QPushButton *m_runButton = nullptr;
    QCheckBox *m_autoRun = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_clearConsoleButton = nullptr;
    QPushButton *m_openBrowserButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QTabWidget *m_outputTabs = nullptr;
    QLabel *m_previewPlaceholder = nullptr;
    QListWidget *m_console = nullptr;
    QTimer *m_autoRunTimer = nullptr;
    WebPlaygroundEditor *m_editor = nullptr;
    WebPlaygroundPreview *m_preview = nullptr;
    quint64 m_runGeneration = 0;
    QElapsedTimer m_consoleRateTimer;
    int m_consoleMessagesThisSecond = 0;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_WEBPLAYGROUNDPANEL_H
