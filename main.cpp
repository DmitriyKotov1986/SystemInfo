#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>
#include "tsysteminfo.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("SystemInfo");
    QCoreApplication::setOrganizationName("OOO SA");
    QCoreApplication::setApplicationVersion("0.1");

    QTimer Timer;

    QCommandLineParser parser;
    parser.setApplicationDescription("Programm for gettin system information");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption Interval(QStringList() << "interval", "Interval beetwen getting information", "IntervalValue", "10000");
    parser.addOption(Interval);

    QCommandLineOption Config(QStringList() << "config", "Config file name", "ConfigFileNameValue", "SystemInfo.ini");
    parser.addOption(Config);

    QCommandLineOption Repeat(QStringList() << "repeat", "Repeat getting informatione every [--interval] msec ", "RepeatValue", "false");
    parser.addOption(Repeat);

    parser.process(a);

    Timer.setInterval(parser.value(Interval).toUInt());
    bool SingleShotTimer = parser.value(Repeat).toLower() == "true" ? false : true;
    Timer.setSingleShot(SingleShotTimer);

    QString ConfigFileName = parser.value(Config);
    if (!parser.isSet(Config))
        ConfigFileName = a.applicationDirPath() +"/" + parser.value(Config);

    TSystemInfo SystemInfo(ConfigFileName);

    QObject::connect(&Timer, SIGNAL(timeout()), &SystemInfo, SLOT(StartGetInformation()));
    //если выполняем чтение один раз то добавляем событие для выхода
    if (SingleShotTimer) QObject::connect(&SystemInfo, SIGNAL(GetInformationComplite()), &a, SLOT(quit()));

    Timer.start();

    return a.exec();
}
