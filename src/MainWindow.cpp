#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    loadSettings();

    connect(&m_controller, &HotspotController::devicesChanged, this, [this](const QVector<WifiDevice> &devices) {
        const QString selected = m_deviceCombo->currentData().toString();
        m_deviceCombo->clear();

        for (const WifiDevice &device : devices) {
            m_deviceCombo->addItem(QStringLiteral("%1 (%2)").arg(device.name, device.state), device.name);
        }

        const int index = m_deviceCombo->findData(selected);
        if (index >= 0) {
            m_deviceCombo->setCurrentIndex(index);
        }

        m_startButton->setEnabled(m_deviceCombo->count() > 0);
        m_controller.refreshStatus(m_connectionNameEdit->text());
    });

    connect(&m_controller, &HotspotController::statusChanged, this, [this](const QString &status, bool active) {
        m_hotspotActive = active;
        m_statusLabel->setText(status);
        m_stopButton->setEnabled(active);
        if (!active) {
            m_startButton->setEnabled(m_deviceCombo->count() > 0);
        }
    });

    connect(&m_controller, &HotspotController::operationStarted, this, [this](const QString &message) {
        setBusy(true, message);
    });

    connect(&m_controller, &HotspotController::operationFinished, this, [this](const QString &message) {
        setBusy(false, message);
    });

    connect(&m_controller, &HotspotController::operationFailed, this, [this](const QString &message) {
        setBusy(false);
        QMessageBox::warning(this, tr("NetworkManager"), message);
        m_controller.refreshStatus(m_connectionNameEdit->text());
    });

    connect(m_refreshButton, &QPushButton::clicked, this, [this] {
        m_controller.refreshDevices();
    });

    connect(m_startButton, &QPushButton::clicked, this, [this] {
        if (!validateConfig()) {
            return;
        }

        saveSettings();
        m_controller.startHotspot(currentConfig());
    });

    connect(m_stopButton, &QPushButton::clicked, this, [this] {
        m_controller.stopHotspot(m_connectionNameEdit->text());
    });

    connect(m_connectionNameEdit, &QLineEdit::editingFinished, this, [this] {
        m_controller.refreshStatus(m_connectionNameEdit->text());
    });

    m_controller.refreshDevices();
}

void MainWindow::buildUi()
{
    setWindowTitle(tr("Hotspot Manager"));
    resize(520, 430);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    m_statusLabel = new QLabel(tr("Status wird geladen..."), central);
    m_statusLabel->setFrameShape(QFrame::StyledPanel);
    m_statusLabel->setMargin(10);
    layout->addWidget(m_statusLabel);

    auto *settingsBox = new QGroupBox(tr("Hotspot-Einstellungen"), central);
    auto *form = new QFormLayout(settingsBox);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_deviceCombo = new QComboBox(settingsBox);
    m_refreshButton = new QPushButton(settingsBox);
    m_refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
    m_refreshButton->setToolTip(tr("Geräte aktualisieren"));

    auto *deviceRow = new QWidget(settingsBox);
    auto *deviceLayout = new QHBoxLayout(deviceRow);
    deviceLayout->setContentsMargins(0, 0, 0, 0);
    deviceLayout->addWidget(m_deviceCombo, 1);
    deviceLayout->addWidget(m_refreshButton);
    form->addRow(tr("WLAN-Gerät"), deviceRow);

    m_connectionNameEdit = new QLineEdit(settingsBox);
    m_connectionNameEdit->setText(QStringLiteral("KDE-Hotspot"));
    form->addRow(tr("Verbindungsname"), m_connectionNameEdit);

    m_ssidEdit = new QLineEdit(settingsBox);
    m_ssidEdit->setText(QStringLiteral("KDE Hotspot"));
    form->addRow(tr("Netzwerkname"), m_ssidEdit);

    m_securityCombo = new QComboBox(settingsBox);
    m_securityCombo->addItems({QStringLiteral("WPA2-Personal"), QStringLiteral("Offen")});
    form->addRow(tr("Sicherheit"), m_securityCombo);

    m_passwordEdit = new QLineEdit(settingsBox);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(tr("Mindestens 8 Zeichen"));
    form->addRow(tr("Passwort"), m_passwordEdit);

    m_bandCombo = new QComboBox(settingsBox);
    m_bandCombo->addItems({tr("Automatisch"), tr("2,4 GHz"), tr("5 GHz")});
    form->addRow(tr("Band"), m_bandCombo);

    m_channelSpin = new QSpinBox(settingsBox);
    m_channelSpin->setRange(0, 165);
    m_channelSpin->setSpecialValueText(tr("Automatisch"));
    form->addRow(tr("Kanal"), m_channelSpin);

    m_sharedIpv4Check = new QCheckBox(tr("Internetverbindung per IPv4 teilen"), settingsBox);
    m_sharedIpv4Check->setChecked(true);
    form->addRow(QString(), m_sharedIpv4Check);

    m_autoconnectCheck = new QCheckBox(tr("Hotspot-Verbindung automatisch aktivieren"), settingsBox);
    form->addRow(QString(), m_autoconnectCheck);

    layout->addWidget(settingsBox);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->addStretch();

    m_stopButton = new QPushButton(tr("Stoppen"), central);
    m_stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    m_stopButton->setEnabled(false);
    buttonRow->addWidget(m_stopButton);

    m_startButton = new QPushButton(tr("Starten"), central);
    m_startButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    m_startButton->setDefault(true);
    buttonRow->addWidget(m_startButton);

    layout->addLayout(buttonRow);
    setCentralWidget(central);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_connectionNameEdit->setText(settings.value("connectionName", m_connectionNameEdit->text()).toString());
    m_ssidEdit->setText(settings.value("ssid", m_ssidEdit->text()).toString());
    m_passwordEdit->setText(settings.value("password").toString());
    m_securityCombo->setCurrentText(settings.value("security", "WPA2-Personal").toString());
    m_bandCombo->setCurrentText(settings.value("band", tr("Automatisch")).toString());
    m_channelSpin->setValue(settings.value("channel", 0).toInt());
    m_sharedIpv4Check->setChecked(settings.value("sharedIpv4", true).toBool());
    m_autoconnectCheck->setChecked(settings.value("autoconnect", false).toBool());
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue("connectionName", m_connectionNameEdit->text());
    settings.setValue("ssid", m_ssidEdit->text());
    settings.setValue("password", m_passwordEdit->text());
    settings.setValue("security", m_securityCombo->currentText());
    settings.setValue("band", m_bandCombo->currentText());
    settings.setValue("channel", m_channelSpin->value());
    settings.setValue("sharedIpv4", m_sharedIpv4Check->isChecked());
    settings.setValue("autoconnect", m_autoconnectCheck->isChecked());
}

void MainWindow::setBusy(bool busy, const QString &message)
{
    if (!message.isEmpty()) {
        m_statusLabel->setText(message);
    }

    m_refreshButton->setEnabled(!busy);
    m_startButton->setEnabled(!busy && m_deviceCombo->count() > 0);
    m_stopButton->setEnabled(!busy && m_hotspotActive);
}

HotspotConfig MainWindow::currentConfig() const
{
    HotspotConfig config;
    config.ifname = m_deviceCombo->currentData().toString();
    config.connectionName = m_connectionNameEdit->text().trimmed();
    config.ssid = m_ssidEdit->text().trimmed();
    config.password = m_passwordEdit->text();
    config.security = m_securityCombo->currentText();
    config.band = m_bandCombo->currentText();
    config.channel = m_channelSpin->value();
    config.sharedIpv4 = m_sharedIpv4Check->isChecked();
    config.autoconnect = m_autoconnectCheck->isChecked();
    return config;
}

bool MainWindow::validateConfig() const
{
    if (m_deviceCombo->currentData().toString().isEmpty()) {
        QMessageBox::warning(const_cast<MainWindow *>(this), tr("Hotspot"), tr("Kein WLAN-Gerät gefunden."));
        return false;
    }

    if (m_connectionNameEdit->text().trimmed().isEmpty() || m_ssidEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(const_cast<MainWindow *>(this), tr("Hotspot"), tr("Verbindungsname und Netzwerkname dürfen nicht leer sein."));
        return false;
    }

    if (m_securityCombo->currentText() != "Offen" && m_passwordEdit->text().size() < 8) {
        QMessageBox::warning(const_cast<MainWindow *>(this), tr("Hotspot"), tr("Das WPA-Passwort muss mindestens 8 Zeichen lang sein."));
        return false;
    }

    return true;
}
