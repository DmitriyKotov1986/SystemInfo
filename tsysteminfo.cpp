#include "tsysteminfo.h"
#include <QProcess>
#include <QtDebug>
#include <QSettings>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QException>
#include <QRegularExpression>
#include <QtCore5Compat/QTextCodec> //Если этот модуль не найден, нужно установить Qt 5 Compatibility Module
#include <iostream>

uint32_t TSystemInfo::NextKey(const QString & OldKey) { //генерирует имя для нового ключа
    if (!CurrentNumberKey.contains(OldKey)) {
        CurrentNumberKey.insert(OldKey, 0);
        return 0;
    }
    else {
       ++CurrentNumberKey[OldKey];
       return CurrentNumberKey[OldKey];
    }
}

void TSystemInfo::SendLogMsg(uint16_t Code, const QString &Msg)
{
    QString Str(Msg);
    Str.replace(QRegularExpression("'"), "''");
   // qDebug() << Str;
    QSqlQuery QueryLog(DB);
    DB.transaction();
    QString QueryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(Code) + ", "
                        "\'SystemMonitor\', "
                        "\'" + Str +"\'"
                        ")";

     if (!QueryLog.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << QueryLog.lastError().text() << " Query: "<< QueryLog.lastQuery();
        DB.rollback();
        return;
    }
    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        return;
    };
}

void TSystemInfo::Parser(const QString &Group, const QByteArray &str)
{
    QTextStream TS(str);

    while (!TS.atEnd()) {
      QString tmp = TS.readLine();
      if (tmp != "\r") {
        size_t position = tmp.indexOf('=');

        TPathInfo Path;
        Path.Category = Group;
        Path.Name = tmp.left(position);
        Path.Number = NextKey(Path.Category + "/" + Path.Name);

        TInfo Value;
        Value.Value = tmp.right(tmp.length() - position - 1);
        Value.Value.chop(1);
        if (Value.Value == "") Value.Value = "n/a";
        Value.DateTime = QDateTime::currentDateTime();

        if (Info.find(Path) != Info.end()) {
            if (Info[Path].Value != Value.Value) {
                Value.UpDate = true;
                Info[Path].Value = Value.Value;
                qDebug() << "FIND NEW VALUE";
            }
        }
        else  {
        //    qDebug() << "ADD NEW PATH";
            Value.UpDate = true;
            Info.insert(Path, Value);
        }
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
    CurrentNumberKey.clear();
    //получаем путь к файлу с командой wnim
    Config.beginGroup("SYSTEM_INFO");
    QString WMICFileName = Config.value("wmic", "").toString();
    QString CMDCodePage = Config.value("CodePage", "IBM 866").toString();
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
        QTextCodec *codec = QTextCodec::codecForName(CMDCodePage.toUtf8());
        //qDebug() << codec->toUnicode(cmd->readAllStandardOutput());
        Parser(Item, codec->toUnicode(cmd->readAllStandardOutput()).toUtf8());
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
    QString QueryText = "INSERT INTO PARAMS (SENDER, DATE_TIME, CATEGORY, NAME, NUMBER, CURRENT_VALUE) "
                        "VALUES ('SystemInfo', '%1', '%2', '%3', %4, '%5')";

    for (const auto & Item : Info.toStdMap()) {
        if (Item.second.UpDate) {
        //   qDebug() << Item.first << Item.second;
           QueryAdd.bindValue(0, Item.second.DateTime);
           QueryAdd.bindValue(1, Item.first.Category);
           QueryAdd.bindValue(2, Item.first.Name);
           QueryAdd.bindValue(3, Item.first.Number);
           QueryAdd.bindValue(4, Item.second.Value);

            if (!QueryAdd.exec(QueryText
                        .arg(Item.second.DateTime.toString("yyyy-MM-dd HH:mm:ss.zzz"))
                        .arg(Item.first.Category)
                        .arg(Item.first.Name)
                        .arg(Item.first.Number)
                        .arg(Item.second.Value))) {
               DB.rollback();
               QString LastError = "Cannot execute query. Error: " + QueryAdd.lastError().text() + " Query: " + QueryAdd.executedQuery();
               throw std::runtime_error(LastError.toStdString());
           }
        }
    }

    if (!DB.commit()) {
       DB.rollback();
       QString LastError = "Cannot commit transation. Error: " + DB.lastError().text();
       throw std::runtime_error(LastError.toStdString());
    };

    //сбрасываем флаг обновления, если значение поменялось
    for (auto Item : Info.keys()) Info[Item].UpDate = false;
}

void TSystemInfo::StartGetInformation()
{
    try {
        Updata();
        SaveToDB();
    //    Print();
        SendLogMsg(MSG_CODE::CODE_OK, "Getting information is succesfull");
        qDebug() << "OK";
    }
    catch (const std::exception& Ex) {
        qDebug() << "FAIL " + QString(Ex.what());
        SendLogMsg(MSG_CODE::CODE_ERROR, "Getting information is fail. Msg:" + QString(Ex.what()));
    }
    emit  GetInformationComplite();
}

void TSystemInfo::ReadCommand(HANDLE hEvent)
{
    std::string line;
    std::getline(std::cin, line);
    QString Cmd = QString::fromStdString(line);
    if (Cmd == "TEST") {
        if (DB.isOpen()) {
            QSqlQuery QueryTest(DB);
            if (QueryTest.exec("SELECT FIRST 1 1 FROM LOG")) qDebug() << "OK";
            else qDebug() << "FAIL Test request to database is fail. Error: " << QueryTest.lastError();
        }
        else qDebug() << "FAIL Connect to database is close. Error: " << DB.lastError().text();
    }
    else if (Cmd == "QUIT") {
        qDebug() << "OK";
        emit Finished();
    }
}
