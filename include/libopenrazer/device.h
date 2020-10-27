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

#ifndef DEVICE_H
#define DEVICE_H

#include <QDBusInterface>
#include <QObject>
#include <razer_test.h>

namespace libopenrazer {

class Led;

/*!
 * \brief Abstraction for accessing Device objects via D-Bus.
 */
class Device : public QObject
{
    Q_OBJECT
public:
    /*!
     * Returns the DBus object path.
     */
    virtual QDBusObjectPath objectPath() = 0;

    /*!
     * Returns if the device has the specified \a featureStr
     */
    virtual bool hasFeature(const QString &featureStr) = 0;

    /*!
     * Returns the URL of an image that shows this device. Could return an empty string if no image was found.
     */
    virtual QString getDeviceImageUrl() = 0;

    /*!
     * Returns a list of Leds supported on the device.
     */
    virtual QList<Led *> getLeds() = 0;

    /*!
     * Returns the device mode of the device, like `0:0` or `3:0` for normal mode and driver mode respetively.
     *
     * \sa setDeviceMode()
     */
    virtual QString getDeviceMode() = 0;

    /*!
     * Sets the device mode to the specified \a mode_id and \a param.
     * Allowed values are `0x00` (normal mode) and `0x03` (driver mode) for \a mode_id and `0x00` for param.
     *
     * When the device is set to Driver Mode, tilting the mouse wheel stops working, and the 'Up DPI' and 'Down DPI' buttons don't actually do anything. Razer Synapse on Windows is supposed to change the DPI instead of the mouse.
     *
     * Returns if the D-Bus call was successful.
     *
     * \sa getDeviceMode()
     */
    virtual bool setDeviceMode(uchar mode_id, uchar param) = 0;

    /*!
     * Returns the serial number of the device which can be used to identify the device.
     */
    virtual QString getSerial() = 0;

    /*!
     * Returns a human readable device name like `Razer DeathAdder Chroma` or `Razer Kraken 7.1`.
     */
    virtual QString getDeviceName() = 0;

    /*!
     * Returns the type of the device. Could be one of `accessory`, `headset`, `keyboard`, `keypad`, `mouse`, `mousepad` or another type, if added to the daemon.
     */
    virtual QString getDeviceType() = 0;

    /*!
     * Returns the firmware version of the device (e.g. `v1.0`).
     */
    virtual QString getFirmwareVersion() = 0;

    /*!
     * Returns the physical layout of the keyboard (e.g. `de_DE`, `en_US`, `en_GB` or `unknown`)
     */
    virtual QString getKeyboardLayout() = 0;

    /*!
     * Returns the current poll rate, e.g. `125`, `500` or `1000`.
     *
     * \sa setPollRate()
     */
    virtual ushort getPollRate() = 0;

    /*!
     * Sets the poll rate of the mouse to the specified \a pollrate, e.g. `125`, `500` or `1000`.
     *
     * Returns if the D-Bus call was successful.
     *
     * \sa getPollRate()
     */
    virtual bool setPollRate(ushort pollrate) = 0;

    /*!
     * Sets the DPI of the mouse to the specified \a dpi_x for the x-Axis and \a dpi_y for the y-Axis. Maximum value is what is returned by maxDPI().
     *
     * Returns if the D-Bus call was successful.
     *
     * \sa getDPI(), maxDPI()
     */
    virtual bool setDPI(::razer_test::RazerDPI dpi) = 0;

    /*!
     * Returns the DPI of the mouse (e.g. `[800, 800]`).
     *
     * \sa setDPI()
     */
    virtual ::razer_test::RazerDPI getDPI() = 0;

    /*!
     * Sets the DPI stages of the mouse to the specified \a dpiStages.
     * A maximum of 5 stages are possible.
     * The DPI value of each must not exceed the maximum DPI of the device.
     *
     * \sa getDPIStages(), maxDPI()
     */
    virtual bool setDPIStages(QVector<::razer_test::RazerDPI> dpiStages) = 0;

    /*!
     * Returns the currently configured DPI stages of the mouse.
     * A maximum of 5 stages are possible.
     * Ex: [(500, 500), (1500, 1500), (1800, 1800)]
     *
     * \sa setDPIStages(), maxDPI()
     */
    virtual QVector<::razer_test::RazerDPI> getDPIStages() = 0;

    /*!
     * Returns the maximum DPI possible for the device.
     *
     * \sa getDPI(), setDPI()
     */
    virtual ushort maxDPI() = 0;

    /*!
     * Sets the lighting to custom mode (applies effects set from defineCustomFrame()).
     *
     * Returns if the D-Bus call was successful.
     *
     * \sa defineCustomFrame()
     */
    virtual bool displayCustomFrame() = 0;

    /*!
     * Sets the lighting of a key row to the specified \a colorData.
     * \a row is the row in the matrix, \a startColumn the column the \a colorData list starts and \a endColumn where the list ends.
     * Note, that you have to call displayCustomFrame() after setting otherwise the effect won't be displayed (even if you have already called displayCustomFrame() before).
     * Currently the driver only accepts whole rows that are sent.
     *
     * Returns if the D-Bus call was successful.
     *
     * \sa displayCustomFrame()
     */
    virtual bool defineCustomFrame(uchar row, uchar startColumn, uchar endColumn, QVector<QColor> colorData) = 0;

    /*!
     * Returns the dimension of the matrix supported on the device.
     *
     * \sa defineCustomFrame()
     */
    virtual ::razer_test::MatrixDimensions getMatrixDimensions() = 0;
};

namespace openrazer {

class DevicePrivate;
class Led;
class LedPrivate;

class Device : public ::libopenrazer::Device
{
public:
    Device(QDBusObjectPath objectPath);
    ~Device() override;

    QDBusObjectPath objectPath() override;
    bool hasFeature(const QString &featureStr) override;
    QString getDeviceImageUrl() override;
    QList<::libopenrazer::Led *> getLeds() override;
    QString getDeviceMode() override;
    bool setDeviceMode(uchar mode_id, uchar param) override;
    QString getSerial() override;
    QString getDeviceName() override;
    QString getDeviceType() override;
    QString getFirmwareVersion() override;
    QString getKeyboardLayout() override;
    ushort getPollRate() override;
    bool setPollRate(ushort pollrate) override;
    bool setDPI(::razer_test::RazerDPI dpi) override;
    ::razer_test::RazerDPI getDPI() override;
    bool setDPIStages(QVector<::razer_test::RazerDPI> dpiStages) override;
    QVector<::razer_test::RazerDPI> getDPIStages() override;
    ushort maxDPI() override;
    bool displayCustomFrame() override;
    bool defineCustomFrame(uchar row, uchar startColumn, uchar endColumn, QVector<QColor> colorData) override;
    ::razer_test::MatrixDimensions getMatrixDimensions() override;

private:
    DevicePrivate *d;

    friend class Led;
    friend class LedPrivate;
};

}

namespace razer_test {

class DevicePrivate;
class Led;
class LedPrivate;

class Device : public ::libopenrazer::Device
{
public:
    Device(QDBusObjectPath objectPath);
    ~Device() override;

    QDBusObjectPath objectPath() override;
    bool hasFeature(const QString &featureStr) override;
    QString getDeviceImageUrl() override;
    QList<::libopenrazer::Led *> getLeds() override;
    QString getDeviceMode() override;
    bool setDeviceMode(uchar mode_id, uchar param) override;
    QString getSerial() override;
    QString getDeviceName() override;
    QString getDeviceType() override;
    QString getFirmwareVersion() override;
    QString getKeyboardLayout() override;
    ushort getPollRate() override;
    bool setPollRate(ushort pollrate) override;
    bool setDPI(::razer_test::RazerDPI dpi) override;
    ::razer_test::RazerDPI getDPI() override;
    bool setDPIStages(QVector<::razer_test::RazerDPI> dpiStages) override;
    QVector<::razer_test::RazerDPI> getDPIStages() override;
    ushort maxDPI() override;
    bool displayCustomFrame() override;
    bool defineCustomFrame(uchar row, uchar startColumn, uchar endColumn, QVector<QColor> colorData) override;
    ::razer_test::MatrixDimensions getMatrixDimensions() override;

private:
    DevicePrivate *d;

    friend class Led;
    friend class LedPrivate;
};

}

}

#endif // DEVICE_H
