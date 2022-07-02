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

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    QCommandLineParser parser;
    parser.setApplicationDescription("Programm for gettin system information. "
                                     "While the program is running, you can send a TEST command to test the current state or QUIT to shut down");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption Config(QStringList() << "config", "Config file name", "ConfigFileNameValue", "SystemInfo.ini");
    parser.addOption(Config);

    parser.process(a);

    QString ConfigFileName = parser.value(Config);
    if (!parser.isSet(Config))
        ConfigFileName = a.applicationDirPath() +"/" + parser.value(Config);

    qDebug() << "Reading configuration from " +  ConfigFileName;

    TSystemInfo SystemInfo(ConfigFileName, &a);

    //настраиваем таймер
    QTimer Timer;
    Timer.setInterval(0);       //таймер сработает так быстро как только это возможно
    Timer.setSingleShot(true);  //таймер сработает 1 раз


    QObject::connect(&Timer, SIGNAL(timeout()), &SystemInfo, SLOT(onStart()));
    QObject::connect(&SystemInfo, SIGNAL(Finished()), &a, SLOT(quit()));

    Timer.start();

    return a.exec();
}
