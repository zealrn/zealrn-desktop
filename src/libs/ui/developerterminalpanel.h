// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_DEVELOPERTERMINALPANEL_H
#define ZEAL_WIDGETUI_DEVELOPERTERMINALPANEL_H

#include <QWidget>

#include <memory>

class QComboBox;
class QLabel;
class QPushButton;
class QShowEvent;
class QVBoxLayout;

namespace Zeal {

namespace Core {
class Settings;
}

namespace WidgetUi {

class TerminalBackend;
class TerminalView;

class DeveloperTerminalPanel final : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DeveloperTerminalPanel)

public:
    explicit DeveloperTerminalPanel(Core::Settings *settings, QWidget *parent = nullptr);
    ~DeveloperTerminalPanel() override;

signals:
    void closeRequested();

protected:
    void showEvent(QShowEvent *event) override;

private:
    bool acknowledgeSafety();
    void ensureBackend();
    void startSession();
    void chooseWorkingDirectory();
    void openExternalTerminal();
    void applyAppearance();
    void setTerminalFontSize(int size);
    void updateStatus(const QString &status);

    Core::Settings *m_settings = nullptr;
    std::unique_ptr<TerminalBackend> m_backend;
    TerminalView *m_terminalView = nullptr;
    QString m_workingDirectory;

    QComboBox *m_shell = nullptr;
    QLabel *m_workingDirectoryLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_fontSizeLabel = nullptr;
    QLabel *m_backendMessage = nullptr;
    QVBoxLayout *m_terminalLayout = nullptr;
    QPushButton *m_newSessionButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_searchButton = nullptr;
    QPushButton *m_clearButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_pasteButton = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_DEVELOPERTERMINALPANEL_H
