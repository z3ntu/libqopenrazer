/*
 * Copyright (C) 2016-2019  Luca Weiss <luca (at) z3ntu (dot) xyz>
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
 */

#ifndef LED_H
#define LED_H

#include <QDBusInterface>
#include <razer_test.h>

namespace libopenrazer {

/*!
 * \brief Abstraction for accessing Led objects via D-Bus.
 */
class Led : public QObject
{
    Q_OBJECT
public:
    /*!
     * Returns the D-Bus object path of the Led
     */
    virtual QDBusObjectPath getObjectPath() = 0;

    /*!
     * Returns if the led has the specified \a fxStr
     */
    virtual bool hasFx(const QString &fxStr) = 0;

    /*!
     * Returns if the device has the specified \a fx
     */
    virtual bool hasFx(::razer_test::RazerEffect fx) = 0;

    /*!
     * Returns the currently active effect
     */
    virtual ::razer_test::RazerEffect getCurrentEffect() = 0;

    /*!
     * Returns the currently active colors (the list will have at least 3 elements)
     */
    virtual QVector<::razer_test::RGB> getCurrentColors() = 0;

    /*!
     * Returns the wave direction of this Led
     */
    virtual ::razer_test::WaveDirection getWaveDirection() = 0;

    /*!
     * Returns the Led ID of this Led
     */
    virtual ::razer_test::RazerLedId getLedId() = 0;

    /*!
     * Sets the LED to none / off.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setOff() = 0;

    /*!
     * Sets the LED to on.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setOn() = 0;

    /*!
     * Sets the lighting to static lighting in the specified \a color.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setStatic(QColor color) = 0;

    /*!
     * Sets the lighting to the single breath effect with the specified \a color.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setBreathing(QColor color) = 0;

    /*!
     * Sets the lighting to the dual breath effect with the specified \a color and \a color2.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setBreathingDual(QColor color, QColor color2) = 0;

    /*!
     * Sets the lighting wheel to the random breath effect.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setBreathingRandom() = 0;

    /*!
     * Sets the lighting wheel to the random breath effect.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setBlinking(QColor color) = 0;

    /*!
     * Sets the lighting to spectrum mode.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setSpectrum() = 0;

    /*!
     * Sets the lighting effect to wave, in the direction \a direction.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setWave(::razer_test::WaveDirection direction) = 0;

    /*!
     * Sets the lighting to reactive mode with the specified \a color and \a speed.
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setReactive(QColor color, ::razer_test::ReactiveSpeed speed) = 0;

    /*!
     * Sets the \a brightness (`0` - `255`).
     *
     * Returns if the D-Bus call was successful.
     */
    virtual bool setBrightness(uchar brightness) = 0;

    /*!
     * Returns the current brightness (`0` - `255`).
     */
    virtual uchar getBrightness() = 0;
};

namespace openrazer {

class Device;
class LedPrivate;
class Led : public ::libopenrazer::Led
{
public:
    Led(Device *device, QDBusObjectPath objectPath, ::razer_test::RazerLedId ledId, QString lightingLocation);
    ~Led() override;

    QDBusObjectPath getObjectPath() override;
    bool hasFx(const QString &fxStr) override;
    bool hasFx(::razer_test::RazerEffect fx) override;
    ::razer_test::RazerEffect getCurrentEffect() override;
    QVector<::razer_test::RGB> getCurrentColors() override;
    ::razer_test::WaveDirection getWaveDirection() override;
    ::razer_test::RazerLedId getLedId() override;
    bool setOff() override;
    bool setOn() override;
    bool setStatic(QColor color) override;
    bool setBreathing(QColor color) override;
    bool setBreathingDual(QColor color, QColor color2) override;
    bool setBreathingRandom() override;
    bool setBlinking(QColor color) override;
    bool setSpectrum() override;
    bool setWave(::razer_test::WaveDirection direction) override;
    bool setReactive(QColor color, ::razer_test::ReactiveSpeed speed) override;
    bool setBrightness(uchar brightness) override;
    uchar getBrightness() override;

private:
    LedPrivate *d;
};

}

namespace razer_test {

class Device;
class LedPrivate;
class Led : public ::libopenrazer::Led
{
public:
    Led(Device *device, QDBusObjectPath objectPath);
    ~Led() override;

    QDBusObjectPath getObjectPath() override;
    bool hasFx(const QString &fxStr) override;
    bool hasFx(::razer_test::RazerEffect fx) override;
    ::razer_test::RazerEffect getCurrentEffect() override;
    QVector<::razer_test::RGB> getCurrentColors() override;
    ::razer_test::WaveDirection getWaveDirection() override;
    ::razer_test::RazerLedId getLedId() override;
    bool setOff() override;
    bool setOn() override;
    bool setStatic(QColor color) override;
    bool setBreathing(QColor color) override;
    bool setBreathingDual(QColor color, QColor color2) override;
    bool setBreathingRandom() override;
    bool setBlinking(QColor color) override;
    bool setSpectrum() override;
    bool setWave(::razer_test::WaveDirection direction) override;
    bool setReactive(QColor color, ::razer_test::ReactiveSpeed speed) override;
    bool setBrightness(uchar brightness) override;
    uchar getBrightness() override;

private:
    LedPrivate *d;
};

}

}

#endif // LED_H
