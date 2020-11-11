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

#include "device_p.h"
#include "libopenrazer.h"
#include "libopenrazer_private.h"

#include <QColor>
#include <QDBusReply>
#include <QDomDocument>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>

namespace libopenrazer {

namespace openrazer {

Device::Device(QDBusObjectPath objectPath)
{
    d = new DevicePrivate();
    d->mParent = this;
    d->mObjectPath = objectPath;

    d->introspect();
    d->setupCapabilities();

    QMap<::razer_test::RazerLedId, QString>::const_iterator i = d->supportedLeds.constBegin();
    while (i != d->supportedLeds.constEnd()) {
        Led *led = new Led(this, d->mObjectPath, i.key(), i.value());
        d->leds.append(led);
        ++i;
    }
}

Device::~Device()
{
    foreach (libopenrazer::Led *led, d->leds) {
        delete led;
    }
}

void DevicePrivate::introspect()
{
    QStringList intr;

    QDBusMessage m = QDBusMessage::createMethodCall("org.razer", mObjectPath.path(), "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusReply<QString> reply = OPENRAZER_DBUS_BUS.call(m);
    if (!reply.isValid()) {
        throw DBusException(reply.error());
    }
    QDomDocument doc;
    doc.setContent(reply.value());

    QDomNodeList nodes = doc.documentElement().childNodes();
    for (int i = 0; i < nodes.count(); i++) {
        // Check if "interface" and also not org.freedesktop.DBus.Introspectable
        QDomElement element = nodes.at(i).toElement();
        QString interfacename = element.attributeNode("name").value();

        QDomNodeList methodnodes = element.childNodes();
        for (int ii = 0; ii < methodnodes.count(); ii++) {
            QDomElement methodelement = methodnodes.at(ii).toElement();
            intr.append(interfacename + ";" + methodelement.attributeNode("name").value());
        }
        intr.append(interfacename);
    }
    introspection = intr;
}

/**
 * Fill "capabilities" list with the capabilities of the device. Names are from the pylib, parsed with the script ./scripts/capabilities_to_cpp.sh in the root of this repo.
 */
void DevicePrivate::setupCapabilities()
{
    if (hasCapabilityInternal("razer.device.misc", "getKeyboardLayout"))
        supportedFeatures.append("keyboard_layout");
    if (hasCapabilityInternal("razer.device.dpi", "setDPI"))
        supportedFeatures.append("dpi");
    if (hasCapabilityInternal("razer.device.dpi", "setDPIStages"))
        supportedFeatures.append("dpi_stages");
    if (hasCapabilityInternal("razer.device.misc", "setPollRate"))
        supportedFeatures.append("poll_rate");
    if (hasCapabilityInternal("razer.device.lighting.chroma", "setCustom"))
        supportedFeatures.append("custom_frame");

    // razer.device.lighting.chroma more than only the normal fx, so check for methods directly
    if (hasCapabilityInternal("razer.device.lighting.chroma", "setNone")
        || hasCapabilityInternal("razer.device.lighting.chroma", "setStatic")
        || hasCapabilityInternal("razer.device.lighting.bw2013"))
        supportedLeds.insert(::razer_test::RazerLedId::Unspecified, "Chroma");
    if (hasCapabilityInternal("razer.device.lighting.logo"))
        supportedLeds.insert(::razer_test::RazerLedId::LogoLED, "Logo");
    if (hasCapabilityInternal("razer.device.lighting.scroll"))
        supportedLeds.insert(::razer_test::RazerLedId::ScrollWheelLED, "Scroll");
    if (hasCapabilityInternal("razer.device.lighting.backlight"))
        supportedLeds.insert(::razer_test::RazerLedId::BacklightLED, "Backlight");
    if (hasCapabilityInternal("razer.device.lighting.left"))
        supportedLeds.insert(::razer_test::RazerLedId::LeftSideLED, "Left");
    if (hasCapabilityInternal("razer.device.lighting.right"))
        supportedLeds.insert(::razer_test::RazerLedId::RightSideLED, "Right");
}

/**
 * Internal method to determine whether a device has a given capability based on interface and method names.
 */
bool DevicePrivate::hasCapabilityInternal(const QString &interface, const QString &method)
{
    if (method.isNull()) {
        return introspection.contains(interface);
    }
    return introspection.contains(interface + ";" + method);
}

QDBusObjectPath Device::objectPath()
{
    return d->mObjectPath;
}

bool Device::hasFeature(const QString &featureStr)
{
    return d->supportedFeatures.contains(featureStr);
}

QString Device::getDeviceImageUrl()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getRazerUrls");
    QString json = handleDBusReply(reply, Q_FUNC_INFO);
    return QJsonDocument::fromJson(json.toUtf8()).object().value("top_img").toString();
}

// ----- DBUS METHODS -----

QList<::libopenrazer::Led *> Device::getLeds()
{
    return d->leds;
}

QString Device::getDeviceMode()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getDeviceMode");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

bool Device::setDeviceMode(uchar mode_id, uchar param)
{
    QDBusReply<void> reply = d->deviceMiscIface()->call("setDeviceMode");
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

QString Device::getSerial()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getSerial");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

QString Device::getDeviceName()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getDeviceName");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

QString Device::getDeviceType()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getDeviceType");
    QString type = handleDBusReply(reply, Q_FUNC_INFO);
    const QHash<QString, QString> translationTable = {
        { "core", "accessory" },
        { "mousemat", "mousepad" },
        { "mug", "accessory" },
    };
    if (translationTable.contains(type))
        return translationTable.value(type);
    return type;
}

QString Device::getFirmwareVersion()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getFirmware");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

QString Device::getKeyboardLayout()
{
    QDBusReply<QString> reply = d->deviceMiscIface()->call("getKeyboardLayout");
    QString layout = handleDBusReply(reply, Q_FUNC_INFO);
    const QHash<QString, QString> translationTable = {
        { "de_DE", "German" },
        { "el_GR", "Greek" },
        { "en_GB", "UK" },
        { "en_US", "US" },
        { "en_US_mac", "US-mac" },
        { "es_ES", "Spanish" },
        { "fr_FR", "French" },
        { "it_IT", "Italian" },
        { "ja_JP", "Japanese" },
        { "pt_PT", "Portuguese" },
    };
    if (translationTable.contains(layout))
        return translationTable.value(layout);
    return layout;
}

ushort Device::getPollRate()
{
    QDBusReply<int> reply = d->deviceMiscIface()->call("getPollRate");
    if (reply.isValid()) {
        return reply.value();
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

bool Device::setPollRate(ushort pollrate)
{
    QDBusReply<void> reply = d->deviceMiscIface()->call("setPollRate", QVariant::fromValue(pollrate));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Device::setDPI(::razer_test::RazerDPI dpi)
{
    QDBusReply<void> reply = d->deviceDpiIface()->call("setDPI", QVariant::fromValue(dpi.dpi_x), QVariant::fromValue(dpi.dpi_y));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

::razer_test::RazerDPI Device::getDPI()
{
    QDBusReply<QList<int>> reply = d->deviceDpiIface()->call("getDPI");
    if (reply.isValid()) {
        QList<int> dpi = reply.value();
        if (dpi.size() == 1) {
            return { static_cast<ushort>(dpi[0]), 0 };
        } else if (dpi.size() == 2) {
            return { static_cast<ushort>(dpi[0]), static_cast<ushort>(dpi[1]) };
        } else {
            throw DBusException("Invalid return array from DPI", "The DPI return array has an invalid size.");
        }
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

bool Device::setDPIStages(uchar activeStage, QVector<::razer_test::RazerDPI> dpiStages)
{
    QDBusReply<void> reply = d->deviceDpiIface()->call("setDPIStages", QVariant::fromValue(activeStage), QVariant::fromValue(dpiStages));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

QPair<uchar, QVector<::razer_test::RazerDPI>> Device::getDPIStages()
{
    QDBusReply<QPair<uchar, QVector<::razer_test::RazerDPI>>> reply = d->deviceDpiIface()->call("getDPIStages");
    return handleDBusReply(reply, Q_FUNC_INFO);
}

ushort Device::maxDPI()
{
    QDBusReply<int> reply = d->deviceDpiIface()->call("maxDPI");
    if (reply.isValid()) {
        return reply.value();
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

bool Device::displayCustomFrame()
{
    QDBusReply<void> reply = d->deviceLightingChromaIface()->call("setCustom");
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Device::defineCustomFrame(uchar row, uchar startColumn, uchar endColumn, QVector<QColor> colorData)
{
    QByteArray data;
    data.append(row);
    data.append(startColumn);
    data.append(endColumn);
    foreach (const QColor &color, colorData) {
        data.append(color.red());
        data.append(color.green());
        data.append(color.blue());
    }
    QDBusReply<void> reply = d->deviceLightingChromaIface()->call("setKeyRow", data);
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

::razer_test::MatrixDimensions Device::getMatrixDimensions()
{
    QDBusReply<QList<int>> reply = d->deviceMiscIface()->call("getMatrixDimensions");
    QList<int> dims = handleDBusReply(reply, Q_FUNC_INFO);
    if (dims.size() != 2)
        throw DBusException("Invalid return array from getMatrixDimensions", "The getMatrixDimensions return array has an invalid size.");
    return { static_cast<uchar>(dims[0]), static_cast<uchar>(dims[1]) };
}

QDBusInterface *DevicePrivate::deviceMiscIface()
{
    if (ifaceMisc == nullptr) {
        ifaceMisc = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), "razer.device.misc",
                                       OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceMisc->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceMisc;
}

QDBusInterface *DevicePrivate::deviceDpiIface()
{
    if (ifaceDpi == nullptr) {
        ifaceDpi = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), "razer.device.dpi",
                                      OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceDpi->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceDpi;
}

QDBusInterface *DevicePrivate::deviceLightingChromaIface()
{
    if (ifaceLightingChroma == nullptr) {
        ifaceLightingChroma = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), "razer.device.lighting.chroma",
                                                 OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceLightingChroma->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceLightingChroma;
}

}

}
