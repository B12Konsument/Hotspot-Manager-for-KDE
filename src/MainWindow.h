#pragma once

#include "HotspotController.h"

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void loadSettings();
    void saveSettings() const;
    void setBusy(bool busy, const QString &message = {});
    HotspotConfig currentConfig() const;
    bool validateConfig() const;

    HotspotController m_controller;
    QComboBox *m_deviceCombo = nullptr;
    QLineEdit *m_connectionNameEdit = nullptr;
    QLineEdit *m_ssidEdit = nullptr;
    QLineEdit *m_passwordEdit = nullptr;
    QComboBox *m_securityCombo = nullptr;
    QComboBox *m_bandCombo = nullptr;
    QSpinBox *m_channelSpin = nullptr;
    QCheckBox *m_sharedIpv4Check = nullptr;
    QCheckBox *m_autoconnectCheck = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_startButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    bool m_hotspotActive = false;
};
