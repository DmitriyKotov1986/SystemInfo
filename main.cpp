#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("SystemInfo");
    QCoreApplication::setOrganizationName("OOO SA");
    QCoreApplication::setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Programm for gettin system information");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption Interval(QStringList() << "interval", "Interval beetwen getting information", "IntervalValue", "0");
    parser.addOption(Interval);

    parser.process(a);

    bool isExecutable = parser.isSet(Interval);
    if (isExecutable) {
      qDebug() << parser.value(Interval);
    }
    else {
      qDebug() << "asdfasfasfas";
    }

    QTimer *Timer = new QTimer();

    a.


    Timer->setInterval()


    return a.exec();
}
