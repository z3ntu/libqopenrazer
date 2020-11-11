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
#include "led_p.h"
#include "libopenrazer.h"
#include "libopenrazer_private.h"

#include <QColor>
#include <QDBusReply>

#define QCOLOR_TO_QVARIANT(c)               \
    QVariant::fromValue(c.red()),           \
            QVariant::fromValue(c.green()), \
            QVariant::fromValue(c.blue())

namespace libopenrazer {

namespace openrazer {

Led::Led(Device *device, QDBusObjectPath objectPath, ::razer_test::RazerLedId ledId, QString lightingLocation)
{
    d = new LedPrivate();
    d->mParent = this;
    d->device = device;
    d->mObjectPath = objectPath;
    d->ledId = ledId;
    d->lightingLocation = lightingLocation;

    // Leave lightingLocationMethod empty in case it's Chroma
    if (lightingLocation == "Chroma") {
        d->interface = "razer.device.lighting.chroma";
    } else {
        d->lightingLocationMethod = lightingLocation;
        d->interface = "razer.device.lighting." + lightingLocation.toLower();
    }

    d->setupCapabilities();
}

/*
 * Destructor
 */
Led::~Led() = default;

void LedPrivate::setupCapabilities()
{
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "None"))
        supportedFx.append("off");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Static"))
        supportedFx.append("static");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Blinking"))
        supportedFx.append("blinking");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "BreathSingle"))
        supportedFx.append("breathing");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "BreathDual"))
        supportedFx.append("breathing_dual");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "BreathRandom"))
        supportedFx.append("breathing_random");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Spectrum"))
        supportedFx.append("spectrum");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Wave"))
        supportedFx.append("wave");
    if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Reactive"))
        supportedFx.append("reactive");

    if (supportedFx.isEmpty() && device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Active")) {
        supportedFx.append("off");
        supportedFx.append("on");
    }

    // No-color static/breathing variants
    if (device->d->hasCapabilityInternal("razer.device.lighting.bw2013", "setStatic"))
        supportedFx.append("static");
    if (device->d->hasCapabilityInternal("razer.device.lighting.bw2013", "setPulsate"))
        supportedFx.append("breathing");

    if (lightingLocation == "Chroma") {
        if (device->d->hasCapabilityInternal("razer.device.lighting.brightness", "setBrightness"))
            supportedFx.append("brightness");
    } else {
        if (device->d->hasCapabilityInternal(interface, "set" + lightingLocationMethod + "Brightness"))
            supportedFx.append("brightness");
    }
}

QDBusObjectPath Led::getObjectPath()
{
    return d->mObjectPath;
}

bool Led::hasFx(const QString &fxStr)
{
    return d->supportedFx.contains(fxStr);
}

bool Led::hasFx(::razer_test::RazerEffect fx)
{
    const QHash<::razer_test::RazerEffect, QString> EffectToString {
        { ::razer_test::RazerEffect::Off, "off" },
        { ::razer_test::RazerEffect::On, "on" },
        { ::razer_test::RazerEffect::Static, "static" },
        { ::razer_test::RazerEffect::Blinking, "blinking" },
        { ::razer_test::RazerEffect::Breathing, "breathing" },
        { ::razer_test::RazerEffect::BreathingDual, "breathing_dual" },
        { ::razer_test::RazerEffect::BreathingRandom, "breathing_random" },
        { ::razer_test::RazerEffect::Spectrum, "spectrum" },
        { ::razer_test::RazerEffect::Wave, "wave" },
        { ::razer_test::RazerEffect::Reactive, "reactive" }
    };
    return hasFx(EffectToString.value(fx));
}

::razer_test::RazerEffect Led::getCurrentEffect()
{
    const QHash<QString, ::razer_test::RazerEffect> StringToEffect {
        { "none", ::razer_test::RazerEffect::Off },
        { "static", ::razer_test::RazerEffect::Static },
        { "blinking", ::razer_test::RazerEffect::Blinking },
        { "pulsate", ::razer_test::RazerEffect::Breathing },
        { "breathSingle", ::razer_test::RazerEffect::Breathing },
        { "breathDual", ::razer_test::RazerEffect::BreathingDual },
        { "breathRandom", ::razer_test::RazerEffect::BreathingRandom },
        { "spectrum", ::razer_test::RazerEffect::Spectrum },
        { "wave", ::razer_test::RazerEffect::Wave },
        { "reactive", ::razer_test::RazerEffect::Reactive },
        // TODO starlightSingle, starlightDual, starlightRandom
        // TODO breathTriple, ripple, rippleRandomColour
    };

    if (d->device->d->hasCapabilityInternal(d->interface, "get" + d->lightingLocationMethod + "Effect")) {
        QDBusReply<QString> reply = d->ledIface()->call("get" + d->lightingLocationMethod + "Effect");
        if (reply.isValid()) {
            return StringToEffect.value(reply.value());
        } else {
            printDBusError(reply.error(), Q_FUNC_INFO);
            throw DBusException(reply.error());
        }
    } else if (d->device->d->hasCapabilityInternal(d->interface, "get" + d->lightingLocationMethod + "Active")) {
        QDBusReply<bool> reply = d->ledIface()->call("get" + d->lightingLocationMethod + "Active");
        if (reply.isValid()) {
            return reply.value() ? ::razer_test::RazerEffect::On : ::razer_test::RazerEffect::Off;
        } else {
            printDBusError(reply.error(), Q_FUNC_INFO);
            throw DBusException(reply.error());
        }
    } else {
        qWarning("libopenrazer: Unhandled branch in getCurrentEffect! Returning default value Off.");
        return ::razer_test::RazerEffect::Off;
    }
}

QVector<::razer_test::RGB> Led::getCurrentColors()
{
    // Devices with only get*Active & set*Active don't have get*EffectColors
    if (!d->device->d->hasCapabilityInternal(d->interface, "get" + d->lightingLocationMethod + "EffectColors")) {
        return { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
    }
    QDBusReply<QByteArray> reply;
    reply = d->ledIface()->call("get" + d->lightingLocationMethod + "EffectColors");
    if (reply.isValid()) {
        auto val = reply.value();
        return { { static_cast<uchar>(val[0]), static_cast<uchar>(val[1]), static_cast<uchar>(val[2]) },
                 { static_cast<uchar>(val[3]), static_cast<uchar>(val[4]), static_cast<uchar>(val[5]) },
                 { static_cast<uchar>(val[6]), static_cast<uchar>(val[7]), static_cast<uchar>(val[8]) } };
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

::razer_test::WaveDirection Led::getWaveDirection()
{
    QDBusReply<int> reply;
    reply = d->ledIface()->call("get" + d->lightingLocationMethod + "WaveDir");
    if (reply.isValid()) {
        switch (reply.value()) {
        case 1:
            return ::razer_test::WaveDirection::LEFT_TO_RIGHT;
        case 2:
            return ::razer_test::WaveDirection::RIGHT_TO_LEFT;
        default:
            throw DBusException("error", QString("failed to convert int %1 to WaveDirection").arg(reply.value()));
        }
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

::razer_test::RazerLedId Led::getLedId()
{
    return d->ledId;
}

bool Led::setOff()
{
    QDBusReply<void> reply;
    if (d->device->d->hasCapabilityInternal(d->interface, "set" + d->lightingLocationMethod + "Active"))
        reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Active", false);
    else
        reply = d->ledIface()->call("set" + d->lightingLocationMethod + "None");
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setOn()
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Active", true);
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setStatic(QColor color)
{
    QDBusReply<void> reply;
    if (d->device->d->hasCapabilityInternal("razer.device.lighting.bw2013", "setStatic"))
        reply = d->ledBw2013Iface()->call("setStatic");
    else
        reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Static", QCOLOR_TO_QVARIANT(color));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setBreathing(QColor color)
{
    QDBusReply<void> reply;
    if (d->device->d->hasCapabilityInternal("razer.device.lighting.bw2013", "setPulsate"))
        reply = d->ledBw2013Iface()->call("setPulsate");
    else
        reply = d->ledIface()->call("set" + d->lightingLocationMethod + "BreathSingle", QCOLOR_TO_QVARIANT(color));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setBreathingDual(QColor color, QColor color2)
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "BreathDual", QCOLOR_TO_QVARIANT(color), QCOLOR_TO_QVARIANT(color2));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setBreathingRandom()
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "BreathRandom");
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setBlinking(QColor color)
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Blinking", QCOLOR_TO_QVARIANT(color));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setSpectrum()
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Spectrum");
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setWave(::razer_test::WaveDirection direction)
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Wave", static_cast<int>(direction));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setReactive(QColor color, ::razer_test::ReactiveSpeed speed)
{
    QDBusReply<void> reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Reactive", QCOLOR_TO_QVARIANT(color), static_cast<uchar>(speed));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

bool Led::setBrightness(uchar brightness)
{
    double dbusBrightness = (double)brightness / 255 * 100;
    QDBusReply<void> reply;
    if (d->lightingLocation == "Chroma")
        reply = d->ledBrightnessIface()->call("setBrightness", QVariant::fromValue(dbusBrightness));
    else
        reply = d->ledIface()->call("set" + d->lightingLocationMethod + "Brightness", QVariant::fromValue(dbusBrightness));
    return handleVoidDBusReply(reply, Q_FUNC_INFO);
}

uchar Led::getBrightness()
{
    QDBusReply<double> reply;
    if (d->lightingLocation == "Chroma")
        reply = d->ledBrightnessIface()->call("getBrightness");
    else
        reply = d->ledIface()->call("get" + d->lightingLocationMethod + "Brightness");
    if (reply.isValid()) {
        return reply.value() / 100 * 255;
    } else {
        printDBusError(reply.error(), Q_FUNC_INFO);
        throw DBusException(reply.error());
    }
}

QDBusInterface *LedPrivate::ledIface()
{
    if (iface == nullptr) {
        iface = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), interface,
                                   OPENRAZER_DBUS_BUS, mParent);
    }
    if (!iface->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return iface;
}

QDBusInterface *LedPrivate::ledBrightnessIface()
{
    if (ifaceBrightness == nullptr) {
        ifaceBrightness = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), "razer.device.lighting.brightness",
                                             OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceBrightness->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceBrightness;
}

QDBusInterface *LedPrivate::ledBw2013Iface()
{
    if (ifaceBw2013 == nullptr) {
        ifaceBw2013 = new QDBusInterface(OPENRAZER_SERVICE_NAME, mObjectPath.path(), "razer.device.lighting.bw2013",
                                         OPENRAZER_DBUS_BUS, mParent);
    }
    if (!ifaceBw2013->isValid()) {
        fprintf(stderr, "%s\n",
                qPrintable(OPENRAZER_DBUS_BUS.lastError().message()));
    }
    return ifaceBw2013;
}

}

}
