/*
 * Copyright (C) 2016-2018  Luca Weiss <luca (at) z3ntu (dot) xyz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "libopenrazer.h"
#include "libopenrazer_private.h"
#include "manager_p.h"

#include <QDBusMetaType>
#include <QDBusReply>
#include <QFileInfo>
#include <QProcess>

namespace libopenrazer {

namespace openrazer {

Manager::Manager()
{
    d = new ManagerPrivate();
    d->mParent = this;

    // Register the enums with the Qt system
    ::razer_test::registerMetaTypes();

    // TODO commonize
    qRegisterMetaType<QPair<uchar, QVector<::razer_test::RazerDPI>>>("QPair<uchar, QVector<RazerDPI>>");
    qDBusRegisterMetaType<QPair<uchar, QVector<::razer_test::RazerDPI>>>();
}

bool Manager::isDaemonRunning()
{
    QDBusReply<QString> reply = d->managerDaemonIface()->call("version");
    return reply.isValid();
}

QVariantHash Manager::getSupportedDevices()
{
    return QVariantHash(); // FIXME
}

QList<QDBusObjectPath> Manager::getDevices()
{
    QDBusReply<QStringList> reply = d->managerDevicesIface()->call("getDevices");
    QStringList serialList = handleDBusReply(reply, Q_FUNC_INFO);
    QList<QDBusObjectPath> ret;
    foreach (const QString &serial, serialList) {
        ret.append(QDBusObjectPath("/org/razer/device/" + serial));
    }
    return ret;
}

Device *Manager::getDevice(QDBusObjectPath objectPath)
{
    return new Device(objectPath);
}

bool Manager::syncEffects(bool yes)
{
    QDBusReply<void> reply = d->managerDevicesIface()->call("syncEffects", QVariant::fromValue(yes));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Manager::getSyncEffects()
{
    QDBusReply<bool> reply = d->managerDevicesIface()->call("getSyncEffects");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

QString Manager::getDaemonVersion()
{
    QDBusReply<QString> reply = d->managerDaemonIface()->call("version");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

bool Manager::setTurnOffOnScreensaver(bool turnOffOnScreensaver)
{
    QDBusReply<void> reply = d->managerDevicesIface()->call("enableTurnOffOnScreensaver", QVariant::fromValue(turnOffOnScreensaver));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Manager::getTurnOffOnScreensaver()
{
    QDBusReply<bool> reply = d->managerDevicesIface()->call("getOffOnScreensaver");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

DaemonStatus Manager::getDaemonStatus()
{
    // Scenarios to handle:
    // - Command systemctl doesn't exist (e.g. Alpine or Gentoo) - exit code 255
    // - Unit wasn't found (i.e. daemon is not installed - or only an old version) - exit code 1
    // Daemon can be not installed but enabled -.-
    QProcess process;
    process.start("systemctl", QStringList() << "is-enabled"
                                             << "openrazer-daemon.service");
    process.waitForFinished();
    QString output(process.readAllStandardOutput());
    QString error(process.readAllStandardError());
    if (output == "enabled\n")
        return DaemonStatus::Enabled;
    else if (output == "disabled\n")
        return DaemonStatus::Disabled;
    else if (error == "Failed to get unit file state for openrazer-daemon.service: No such file or directory\n")
        return DaemonStatus::NotInstalled;
    else if (process.error() == QProcess::FailedToStart) { // check if systemctl could be started - fails on non-systemd distros and flatpak
        QFileInfo daemonFile("/usr/bin/openrazer-daemon");
        // if the daemon executable does not exist, show the not_installed message - probably flatpak
        if (!daemonFile.exists())
            return DaemonStatus::NotInstalled;
        // otherwise show the NoSystemd message - probably a non-systemd distro
        return DaemonStatus::NoSystemd;
    } else {
        qWarning("libopenrazer: There was an error checking if the daemon is enabled. Unit state is: %s. Error message: %s", qUtf8Printable(output), qUtf8Printable(error));
        return DaemonStatus::Unknown;
    }
}

QString Manager::getDaemonStatusOutput()
{
    QProcess process;
    process.start("systemctl", QStringList() << "status"
                                             << "openrazer-daemon.service");
    process.waitForFinished();
    QString output(process.readAllStandardOutput());
    QString error(process.readAllStandardError());
    // TODO Handle systemctl not found
    // TODO Check if output and error and only display what's filled (to get rid of stray newline)
    return output + "\n" + error;
}

bool Manager::enableDaemon()
{
    QProcess process;
    process.start("systemctl", QStringList() << "enable"
                                             << "openrazer-daemon.service");
    process.waitForFinished();
    return process.exitCode() == 0;
}

// TODO New Qt5 connect style syntax - maybe https://stackoverflow.com/a/35501065/3527128
bool Manager::connectDevicesChanged(QObject *receiver, const char *slot)
{
    bool ret = OPENRAZER_DBUS_BUS.connect(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.devices", "device_added", receiver, slot);
    ret &= OPENRAZER_DBUS_BUS.connect(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.devices", "device_removed", receiver, slot);
    return ret;
}

QDBusInterface *ManagerPrivate::managerDaemonIface()
{
    if (ifaceDaemon == nullptr) {
        ifaceDaemon = new QDBusInterface(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.daemon",
                                         OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceDaemon->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceDaemon;
}

QDBusInterface *ManagerPrivate::managerDevicesIface()
{
    if (ifaceDevices == nullptr) {
        ifaceDevices = new QDBusInterface(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.devices",
                                          OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceDevices->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceDevices;
}

}

}
