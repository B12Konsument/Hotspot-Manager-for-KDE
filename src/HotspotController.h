#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

struct WifiDevice {
    QString name;
    QString state;
};

struct HotspotConfig {
    QString ifname;
    QString connectionName;
    QString ssid;
    QString password;
    QString security;
    QString band;
    int channel = 0;
    bool sharedIpv4 = true;
    bool autoconnect = false;
};

class HotspotController : public QObject
{
    Q_OBJECT

public:
    explicit HotspotController(QObject *parent = nullptr);

    void refreshDevices();
    void refreshStatus(const QString &connectionName);
    void startHotspot(const HotspotConfig &config);
    void stopHotspot(const QString &connectionName);

signals:
    void devicesChanged(const QVector<WifiDevice> &devices);
    void statusChanged(const QString &statusText, bool active);
    void operationStarted(const QString &message);
    void operationFinished(const QString &message);
    void operationFailed(const QString &message);

private:
    using ProcessCallback = std::function<void(int, const QString &, const QString &)>;

    void runNmcli(const QStringList &arguments, ProcessCallback callback);
    void configureAndActivate(const HotspotConfig &config);
    void addConnection(const HotspotConfig &config);
    void modifyConnection(const HotspotConfig &config);
    void activateConnection(const HotspotConfig &config);
    static QString bandValue(const QString &label);
};
