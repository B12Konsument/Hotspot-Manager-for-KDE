#include "HotspotController.h"

#include <QProcess>

HotspotController::HotspotController(QObject *parent)
    : QObject(parent)
{
}

void HotspotController::refreshDevices()
{
    runNmcli({"-t", "-f", "DEVICE,TYPE,STATE", "device"}, [this](int exitCode, const QString &out, const QString &err) {
        if (exitCode != 0) {
            emit operationFailed(err.isEmpty() ? tr("WLAN-Geräte konnten nicht gelesen werden.") : err.trimmed());
            return;
        }

        QVector<WifiDevice> devices;
        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(':');
            if (parts.size() >= 3 && parts.at(1) == "wifi") {
                devices.push_back({parts.at(0), parts.at(2)});
            }
        }

        emit devicesChanged(devices);
    });
}

void HotspotController::refreshStatus(const QString &connectionName)
{
    runNmcli({"-t", "-f", "NAME,TYPE,DEVICE", "connection", "show", "--active"},
             [this, connectionName](int exitCode, const QString &out, const QString &err) {
        if (exitCode != 0) {
            emit statusChanged(err.isEmpty() ? tr("Status unbekannt") : err.trimmed(), false);
            return;
        }

        const QStringList lines = out.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(':');
            if (parts.size() >= 3 && parts.at(0) == connectionName && !parts.at(2).isEmpty()) {
                emit statusChanged(tr("Hotspot aktiv auf %1").arg(parts.at(2)), true);
                return;
            }
        }

        emit statusChanged(tr("Hotspot inaktiv"), false);
    });
}

void HotspotController::startHotspot(const HotspotConfig &config)
{
    emit operationStarted(tr("Hotspot wird vorbereitet..."));

    runNmcli({"-t", "-f", "NAME", "connection", "show"}, [this, config](int exitCode, const QString &out, const QString &) {
        if (exitCode == 0 && out.split('\n', Qt::SkipEmptyParts).contains(config.connectionName)) {
            configureAndActivate(config);
            return;
        }

        addConnection(config);
    });
}

void HotspotController::stopHotspot(const QString &connectionName)
{
    emit operationStarted(tr("Hotspot wird gestoppt..."));
    runNmcli({"connection", "down", connectionName}, [this, connectionName](int exitCode, const QString &, const QString &err) {
        if (exitCode != 0) {
            emit operationFailed(err.isEmpty() ? tr("Hotspot konnte nicht gestoppt werden.") : err.trimmed());
            return;
        }

        emit operationFinished(tr("Hotspot gestoppt."));
        refreshStatus(connectionName);
    });
}

void HotspotController::runNmcli(const QStringList &arguments, ProcessCallback callback)
{
    auto *process = new QProcess(this);
    process->setProgram("nmcli");
    process->setArguments(arguments);

    connect(process, &QProcess::finished, this, [process, callback](int exitCode, QProcess::ExitStatus) {
        const QString out = QString::fromUtf8(process->readAllStandardOutput());
        const QString err = QString::fromUtf8(process->readAllStandardError());
        process->deleteLater();
        callback(exitCode, out, err);
    });

    connect(process, &QProcess::errorOccurred, this, [process, callback](QProcess::ProcessError) {
        const QString message = process->errorString();
        process->deleteLater();
        callback(1, {}, message);
    });

    process->start();
}

void HotspotController::configureAndActivate(const HotspotConfig &config)
{
    modifyConnection(config);
}

void HotspotController::addConnection(const HotspotConfig &config)
{
    runNmcli({"connection", "add",
              "type", "wifi",
              "ifname", config.ifname,
              "con-name", config.connectionName,
              "ssid", config.ssid},
             [this, config](int exitCode, const QString &, const QString &err) {
        if (exitCode != 0) {
            emit operationFailed(err.isEmpty() ? tr("Hotspot-Verbindung konnte nicht angelegt werden.") : err.trimmed());
            return;
        }

        configureAndActivate(config);
    });
}

void HotspotController::modifyConnection(const HotspotConfig &config)
{
    QStringList args = {"connection", "modify", config.connectionName,
                        "connection.interface-name", config.ifname,
                        "connection.autoconnect", config.autoconnect ? "yes" : "no",
                        "802-11-wireless.mode", "ap",
                        "802-11-wireless.ssid", config.ssid,
                        "ipv4.method", config.sharedIpv4 ? "shared" : "disabled",
                        "ipv6.method", "ignore"};

    const QString band = bandValue(config.band);
    if (!band.isEmpty()) {
        args << "802-11-wireless.band" << band;
    }

    if (config.channel > 0) {
        args << "802-11-wireless.channel" << QString::number(config.channel);
    } else {
        args << "802-11-wireless.channel" << "0";
    }

    if (config.security == "WPA2-Personal") {
        args << "wifi-sec.key-mgmt" << "wpa-psk"
             << "wifi-sec.psk" << config.password;
    } else {
        args << "wifi-sec.key-mgmt" << ""
             << "wifi-sec.psk" << "";
    }

    runNmcli(args, [this, config](int exitCode, const QString &, const QString &err) {
        if (exitCode != 0) {
            emit operationFailed(err.isEmpty() ? tr("Hotspot-Einstellungen konnten nicht gespeichert werden.") : err.trimmed());
            return;
        }

        activateConnection(config);
    });
}

void HotspotController::activateConnection(const HotspotConfig &config)
{
    emit operationStarted(tr("Hotspot wird gestartet..."));
    runNmcli({"connection", "up", config.connectionName}, [this, config](int exitCode, const QString &, const QString &err) {
        if (exitCode != 0) {
            emit operationFailed(err.isEmpty() ? tr("Hotspot konnte nicht gestartet werden.") : err.trimmed());
            return;
        }

        emit operationFinished(tr("Hotspot gestartet."));
        refreshStatus(config.connectionName);
    });
}

QString HotspotController::bandValue(const QString &label)
{
    if (label.startsWith("2,4") || label.startsWith("2.4")) {
        return "bg";
    }

    if (label.startsWith("5")) {
        return "a";
    }

    return {};
}
