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

#include <QDBusMetaType>
#include <QDBusReply>
#include <QFileInfo>
#include <QProcess>

namespace libopenrazer {

Manager::Manager()
{
    // Register the enums with the Qt system
    razer_test::registerMetaTypes();
}

/*!
 * \fn bool libopenrazer::isDaemonRunning()
 *
 * Returns if the daemon is running (and responding to the version call).
 */
bool Manager::isDaemonRunning()
{
    QDBusReply<QString> reply = managerDaemonIface()->call("version");
    return reply.isValid();
}

/*!
 * \fn QVariantHash libopenrazer::getSupportedDevices()
 *
 * Returns a list of supported devices in the format of \c {QHash<QString(DeviceName), QList<double(VID), double(PID)>>}.
 *
 * \sa Device::getVid(), Device::getPid()
 */
QVariantHash Manager::getSupportedDevices()
{
    return QVariantHash(); // FIXME
}

/*!
 * \fn QStringList libopenrazer::getConnectedDevices()
 *
 * Returns a list of connected devices in form of their serial number (e.g. \c ['XX0000000001', 'PM1439131641838']).
 *
 * Can be used to create a libopenrazer::Device object and get further information about the device.
 */
QList<QDBusObjectPath> Manager::getDevices()
{
    QDBusReply<QStringList> reply = managerDevicesIface()->call("getDevices");
    QStringList serialList = handleDBusReply(reply, Q_FUNC_INFO);
    QList<QDBusObjectPath> ret;
    foreach (const QString &serial, serialList) {
        ret.append(QDBusObjectPath("/org/razer/device/" + serial));
    }
    return ret;
}

/*!
 * \fn bool libopenrazer::syncEffects(bool yes)
 *
 * If devices should sync effects, as specified by \a yes.
 *
 * Example: Set it to \c 'on', set the lighting on one device to something, other devices connected will automatically get set to the same effect.
 *
 * Returns if the D-Bus call was successful.
 *
 * \sa getSyncEffects()
 */
bool Manager::syncEffects(bool yes)
{
    return false; // FIXME
}

/*!
 * \fn bool libopenrazer::getSyncEffects()
 *
 * Returns if devices should sync effect.
 *
 * \sa syncEffects()
 */
bool Manager::getSyncEffects()
{
    return false; // FIXME
}

/*!
 * \fn QString libopenrazer::getDaemonVersion()
 *
 * Returns the daemon version currently running (e.g. \c '2.3.0').
 */
QString Manager::getDaemonVersion()
{
    QDBusReply<QString> reply = managerDaemonIface()->call("version");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

/*!
 * \fn bool libopenrazer::setTurnOffOnScreensaver(bool turnOffOnScreensaver)
 *
 * Sets if the LEDs should turn off if the screensaver is turned on, as specified by \a turnOffOnScreensaver.
 *
 * Returns if the D-Bus call was successful.
 *
 * \sa getTurnOffOnScreensaver()
 */
bool Manager::setTurnOffOnScreensaver(bool turnOffOnScreensaver)
{
    return false; // FIXME
}

/*!
 * \fn bool libopenrazer::getTurnOffOnScreensaver()
 *
 * Returns if the LEDs should turn off if the screensaver is turned on.
 *
 * \sa setTurnOffOnScreensaver()
 */
bool Manager::getTurnOffOnScreensaver()
{
    return false; // FIXME
}

/*!
 * \fn DaemonStatus libopenrazer::getDaemonStatus()
 *
 * Returns status of the daemon, see DaemonStatus.
 */
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

/*!
 * \fn QString libopenrazer::getDaemonStatusOutput()
 *
 * Returns the multiline output of \c {"systemctl status razer_test.service"}.
 */
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

/*!
 * \fn bool libopenrazer::enableDaemon()
 *
 * Enables the systemd unit for the OpenRazer daemon to auto-start when the user logs in. Runs \c {"systemctl enable razer_test.service"}
 *
 * Returns if the call was successful.
 */
bool Manager::enableDaemon()
{
    QProcess process;
    process.start("systemctl", QStringList() << "enable"
                                             << "razer_test.service");
    process.waitForFinished();
    return process.exitCode() == 0;
}

/*!
 * \fn bool libopenrazer::connectDeviceAdded(QObject *receiver, const char *slot)
 *
 * Connects the \c device_added signal of the daemon to the specified method using the \a receiver and \a slot.
 *
 * Can be used in the Qt4-style Signal&Slot syntax, e.g.:
 * \code
 * libopenrazer::connectDeviceAdded(this, SLOT(deviceAdded()));
 * \endcode
 *
 * Returns if the connection was successful.
 *
 * \sa connectDeviceRemoved()
 */
// TODO New Qt5 connect style syntax - maybe https://stackoverflow.com/a/35501065/3527128
bool Manager::connectDevicesChanged(QObject *receiver, const char *slot)
{
    return RAZER_TEST_DBUS_BUS.connect(OPENRAZER_SERVICE_NAME, "/io/github/openrazer1", "io.github.openrazer1.Manager", "devicesChanged", receiver, slot);
}

QDBusInterface *Manager::managerDaemonIface()
{
    if (ifaceDaemon == nullptr) {
        ifaceDaemon = new QDBusInterface(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.daemon",
                                         RAZER_TEST_DBUS_BUS, this);
    }
    if (!ifaceDaemon->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(RAZER_TEST_DBUS_BUS.lastError().message()));
    }
    return ifaceDaemon;
}

QDBusInterface *Manager::managerDevicesIface()
{
    if (ifaceDevices == nullptr) {
        ifaceDevices = new QDBusInterface(OPENRAZER_SERVICE_NAME, "/org/razer", "razer.devices",
                                          RAZER_TEST_DBUS_BUS, this);
    }
    if (!ifaceDevices->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(RAZER_TEST_DBUS_BUS.lastError().message()));
    }
    return ifaceDevices;
}

}