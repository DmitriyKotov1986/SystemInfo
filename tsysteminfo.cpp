#include "tsysteminfo.h"
#include <QProcess>
#include <QtDebug>
#include <QSettings>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>

QString TSystemInfo::NextKey(const QString & OldKey) { //генерирует имя для нового ключа
    if (!CurrentNumberKey.contains(OldKey)) {
        CurrentNumberKey.insert(OldKey, 0);
        return OldKey + "/0";
    }
    else {
       ++CurrentNumberKey[OldKey];
       return OldKey + "/" + QString::number(CurrentNumberKey[OldKey]);
    }
}

void TSystemInfo::SendLogMsg(uint16_t Code, const QString &Msg)
{
    QSqlQuery QueryLog(DB);
    DB.transaction();
    QString QueryText = "INSERT INTO LOG ( CAT, SENDER, MSG) VALUES ( "
                        + QString::number(Code) + ", "
                        "\'SystemMonitor\', "
                        "\'" + Msg +"\'"
                        ")";

     if (!QueryLog.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << QueryLog.lastError().text();
    }
    DB.commit();
}

void TSystemInfo::Parser(const QString &Group, const QByteArray &str)
{
    QTextStream TS(str);

    while (!TS.atEnd()) {
      QString tmp = TS.readLine();
      if (tmp != "\r") {
        size_t position = tmp.indexOf('=');
        QString Key = tmp.left(position);
        QString Value = tmp.right(tmp.length() - position - 1);
        Value.chop(1);
        Key = NextKey(Group + "/" + Key);
        Info.insert(Key, Value);
      }
    }
}

TSystemInfo::TSystemInfo(const QString &FileName)
    : Config(FileName, QSettings::IniFormat)
{
    Config.beginGroup("DATABASE");

    DB = QSqlDatabase::addDatabase(Config.value("Driver", "QODBC").toString(), "MainDB");
    DB.setDatabaseName(Config.value("DataBase", "SystemMonitorDB").toString());
    DB.setUserName(Config.value("UID", "SYSDBA").toString());
    DB.setPassword(Config.value("PWD", "MASTERKEY").toString());
    DB.setConnectOptions(Config.value("ConnectionOprions", "").toString());
    DB.setPort(Config.value("Port", "3051").toUInt());
    DB.setHostName(Config.value("Host", "localhost").toString());

    Config.endGroup();

    if (!DB.open()) {
        qDebug() << "FAIL. Cannot connect to database. Error: " << DB.lastError().text();
    };

    SendLogMsg(MSG_CODE::CODE_OK, "Start is succesfull");

}

TSystemInfo::~TSystemInfo()
{
    SendLogMsg(MSG_CODE::CODE_OK, "Finished");
    DB.close();
}

void TSystemInfo::Updata()
{
    //получаем путь к файлу с командой wnim
    Config.beginGroup("SYSTEM_INFO");
    QString WMICFileName = Config.value("wmic", "").toString();
    uint32_t GroupListCount = Config.value("GroupCount", 0).toUInt();
    QStringList GroupList;
    for (uint32_t i = 0; i < GroupListCount; ++i) GroupList << Config.value("Group" + QString::number(i), "").toString();
    Config.endGroup();

    for (auto Item : GroupList) {
        Config.beginGroup(Item);
        uint32_t Count = Config.value("Count", 0).toUInt();
        QStringList Keys(Item);
        Keys << "GET";
        for (uint32_t i = 0; i < Count; ++i) {
            Keys << Config.value("Key" + QString::number(i), "").toString();
            if (i != Count - 1) Keys << ",";
        }
        Config.endGroup();
        Keys << "/FORMAT:LIST";

        //выполняем команду
        QProcess *cmd = new QProcess();
        cmd->start(WMICFileName ,Keys);
        cmd->waitForFinished(10000);
        //qDebug() << cmd->readAllStandardOutput();
        Parser(Item, cmd->readAllStandardOutput());
        delete cmd;
    }
}

void TSystemInfo::Print()
{
    for (const auto & Item: Info.toStdMap()) {
        qDebug() << Item.first << Item.second;
    }
}

void TSystemInfo::SaveToDB()
{
    QSqlQuery QueryAdd(DB);
    DB.transaction();
    QString QueryText = "INSERT INTO LOG ( CAT, SENDER, MSG) VALUES ( "
                        + QString::number(Code) + ", "
                        "\'SystemMonitor\', "
                        "\'" + Msg +"\'"
                        ")";

     if (!QueryAdd.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << QueryAdd.lastError().text();
    }
    DB.commit();

}

void TSystemInfo::StartGetInformation()
{
    try {
        Updata();
        SaveToDB();
     //   Print();
        SendLogMsg(MSG_CODE::CODE_OK, "Getting information is succesfull");
        qDebug() << "OK";
    }
    catch (...) {
        qDebug() << "FAIL";
        SendLogMsg(MSG_CODE::CODE_ERROR, "Getting information is fail");
    }
    emit  GetInformationComplite();
}
