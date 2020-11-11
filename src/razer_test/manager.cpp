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
#include <QFileInfo>
#include <QProcess>

namespace libopenrazer {

namespace razer_test {

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
    QVariant reply = d->managerIface()->property("Version");
    return reply.isValid();
}

QVariantHash Manager::getSupportedDevices()
{
    return QVariantHash(); // TODO Needs implementation
}

QList<QDBusObjectPath> Manager::getDevices()
{
    QVariant reply = d->managerIface()->property("Devices");
    return handleDBusVariant<QList<QDBusObjectPath>>(reply, d->managerIface()->lastError(), Q_FUNC_INFO);
}

Device *Manager::getDevice(QDBusObjectPath objectPath)
{
    return new Device(objectPath);
}

bool Manager::syncEffects(bool yes)
{
    return false; // TODO Needs implementation
}

bool Manager::getSyncEffects()
{
    return false; // TODO Needs implementation
}

QString Manager::getDaemonVersion()
{
    QVariant reply = d->managerIface()->property("Version");
    return handleDBusVariant<QString>(reply, d->managerIface()->lastError(), Q_FUNC_INFO);
}

bool Manager::setTurnOffOnScreensaver(bool turnOffOnScreensaver)
{
    return false; // TODO Needs implementation
}

bool Manager::getTurnOffOnScreensaver()
{
    return false; // TODO Needs implementation
}

DaemonStatus Manager::getDaemonStatus()
{
    // Scenarios to handle:
    // - Command systemctl doesn't exist (e.g. Alpine or Gentoo) - exit code 255
    // - Unit wasn't found (i.e. daemon is not installed - or only an old version) - exit code 1
    // Daemon can be not installed but enabled -.-
    QProcess process;
    process.start("systemctl", QStringList() << "is-enabled"
                                             << "razer_test.service");
    process.waitForFinished();
    QString output(process.readAllStandardOutput());
    QString error(process.readAllStandardError());
    if (output == "enabled\n")
        return DaemonStatus::Enabled;
    else if (output == "disabled\n")
        return DaemonStatus::Disabled;
    else if (error == "Failed to get unit file state for razer_test.service: No such file or directory\n")
        return DaemonStatus::NotInstalled;
    else if (process.error() == QProcess::FailedToStart) { // check if systemctl could be started - fails on non-systemd distros and flatpak
        QFileInfo daemonFile("/usr/bin/razer_test");
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
                                             << "razer_test.service");
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
                                             << "razer_test.service");
    process.waitForFinished();
    return process.exitCode() == 0;
}

// TODO New Qt5 connect style syntax - maybe https://stackoverflow.com/a/35501065/3527128
bool Manager::connectDevicesChanged(QObject *receiver, const char *slot)
{
    return OPENRAZER_DBUS_BUS.connect(OPENRAZER_SERVICE_NAME, "/io/github/openrazer1", "io.github.openrazer1.Manager", "devicesChanged", receiver, slot);
}

QDBusInterface *ManagerPrivate::managerIface()
{
    if (iface == nullptr) {
        iface = new QDBusInterface(OPENRAZER_SERVICE_NAME, "/io/github/openrazer1", "io.github.openrazer1.Manager",
                                   OPENRAZER_DBUS_BUS, mParent);
    }
    if (!iface->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return iface;
}

}

}
