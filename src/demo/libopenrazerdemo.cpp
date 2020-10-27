#include "libopenrazer.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

// Main method for testing / playing.
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ "backend", "libopenrazer backend to use (openrazer/razer_test)", "backend" });
    parser.process(app);

    QString chosenBackend = parser.value("backend");

    libopenrazer::Manager *manager;
    if (chosenBackend == "" || chosenBackend == "openrazer") {
        manager = new libopenrazer::openrazer::Manager();
    } else if (chosenBackend == "razer_test") {
        manager = new libopenrazer::razer_test::Manager();
    } else {
        parser.showHelp();
    }

    qDebug() << "Daemon running:" << manager->isDaemonRunning();
    qDebug() << "Daemon version:" << manager->getDaemonVersion();
    qDebug() << "Supported devices:" << manager->getSupportedDevices();
    manager->syncEffects(false);

    foreach (const QDBusObjectPath &devicePath, manager->getDevices()) {
        qDebug() << "=========================";
        libopenrazer::Device *device = manager->getDevice(devicePath);
        qDebug() << "Devicename:" << device->getDeviceName();
        qDebug() << "Firmware version:" << device->getFirmwareVersion();
        // try { // fake driver doesn't have device_mode
        //     qDebug() << "Devicemode:" << device->getDeviceMode();
        // } catch (QException &e) {
        // }
        // device->setDeviceMode(0x03, 0x00);
        // qDebug() << "Devicemode:" << device->getDeviceMode();
        qDebug() << "Serial: " << device->getSerial();

        if (device->hasFeature("dpi")) {
            qDebug() << "-----------";
            qDebug() << "Current DPI:" << device->getDPI();
            device->setDPI({ 500, 500 });
            qDebug() << "Changed DPI:" << device->getDPI();
            qDebug() << "Max DPI: " << device->maxDPI();
        }

        if (device->hasFeature("dpi_stages")) {
            qDebug() << "-----------";
            qDebug() << "Current DPI stages:" << device->getDPIStages();
            device->setDPIStages({ { 400, 500 }, { 600, 700 }, { 800, 900 } });
            qDebug() << "Changed DPI stages:" << device->getDPIStages();
        }

        if (device->hasFeature("poll_rate")) {
            qDebug() << "-----------";
            qDebug() << "Current poll rate:" << device->getPollRate();
            device->setPollRate(1000);
            qDebug() << "Changed poll rate:" << device->getPollRate();
        }

        foreach (libopenrazer::Led *led, device->getLeds()) {
            qDebug() << "-----------";
            QString str = libopenrazer::ledIdToStringTable.value(led->getLedId(), "error");
            if (led->hasFx("brightness")) {
                qDebug() << "getBrightness:" << led->getBrightness();
            }
            razer_test::RazerEffect currentEffect = led->getCurrentEffect();
            QVector<razer_test::RGB> currentColors = led->getCurrentColors();

            qDebug() << "color:" << currentColors[0];

            // Add items from capabilities
            for (auto ledFx : libopenrazer::ledFxList) {
                if (led->hasFx(ledFx.getIdentifier())) {
                    qDebug() << "has effect:" << ledFx.getDisplayString();
                }
            }
        }

        if (device->hasFeature("kbd_layout")) {
            qDebug() << "-----------";
            qDebug() << "Keyboard layout:";
            qDebug() << device->getKeyboardLayout();
        }

        // if(device->hasMatrix()) {
        //     QList<int> dimen = device->getMatrixDimensions();
        //     qDebug() << dimen;
        //     qDebug() << dimen[0] << "-" << dimen[1];
        //     QList<QColor> colors = QList<QColor>();
        //     for(int i=0; i<dimen[1]; i++)
        //         colors << QColor("yellow");
        //     qDebug() << "size:" << colors.size();
        //     for(int i=0; i<dimen[0]; i++) {
        //         qDebug() << i;
        //         device->setKeyRow(i, 0, dimen[1]-1, colors);
        //         device->setCustom();
        //         qDebug() << "Press Enter to continue.";
        //         std::cin.ignore();
        //     }
        // }

        // QHash<QString, bool> hash = device->getAllCapabilities();
        // for (QHash<QString, bool>::iterator i = hash.begin(); i != hash.end(); ++i)
        //     qDebug() << i.key() << ": " << i.value();

        delete device;
    }
}
