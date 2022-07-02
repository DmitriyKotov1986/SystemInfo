#include "tsysteminfo.h"
#include <QProcess>
#include <QtDebug>
#include <QSettings>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QException>
#include <QRegularExpression>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QFile>

TSystemInfo::TSystemInfo(const QString &ConfigFileName, QObject *parent)
    : QObject(parent)
{
    Config = new QSettings(ConfigFileName, QSettings::IniFormat);

    Config->beginGroup("DATABASE");
    DB = QSqlDatabase::addDatabase(Config->value("Driver", "QODBC").toString(), "MainDB");
    DB.setDatabaseName(Config->value("DataBase", "SystemMonitorDB").toString());
    DB.setUserName(Config->value("UID", "SYSDBA").toString());
    DB.setPassword(Config->value("PWD", "MASTERKEY").toString());
    DB.setConnectOptions(Config->value("ConnectionOprions", "").toString());
    DB.setPort(Config->value("Port", "3051").toUInt());
    DB.setHostName(Config->value("Host", "localhost").toString());
    Config->endGroup();

    Config->beginGroup("SYSTEM");
    UpdateTimer.setInterval(Config->value("Interval", "60000").toInt());
    UpdateTimer.setSingleShot(false);
    DebugMode = Config->value("DebugMode", "0").toBool();
    Config->endGroup();
    QObject::connect(&UpdateTimer, SIGNAL(timeout()), this, SLOT(onStartGetData()));

    Config->beginGroup("SERVER");
    HTTPServerInfo.AZSCode = Config->value("UID", "000").toString();
    HTTPServerInfo.Url = "http://" + Config->value("Host", "localhost").toString() + ":" + Config->value("Port", "80").toString() +
                  "/CGI/SYSTEMINFO&" + HTTPServerInfo.AZSCode +"&" + Config->value("PWD", "123456").toString();
    HTTPServerInfo.MaxRecord = Config->value("MaxRecord", "10").toUInt();
    HTTPServerInfo.LastID = Config->value("LastID", "0").toULongLong();
    Config->endGroup();

    AboutSystem = new TAboutSystem(*Config, this);
    QObject::connect(AboutSystem, SIGNAL(SaveToDB(const QString&,  const QString&, const QString&, const uint16_t, const QString&)),
                     this, SLOT(onSaveToDB(const QString&,  const QString&, const QString&, const uint16_t, const QString&)));
    QObject::connect(AboutSystem, SIGNAL(GetDataComplite()), this, SLOT(onGetDataComplite()));

    HTTPQuery = new THTTPQuery(HTTPServerInfo.Url, this);
    QObject::connect(HTTPQuery, SIGNAL(GetAnswer(const QByteArray &)), this, SLOT(onHTTPGetAnswer(const QByteArray &)));
    QObject::connect(HTTPQuery, SIGNAL(SendLogMsg(uint16_t, const QString &)), this, SLOT(onSendLogMsg(uint16_t, const QString &)));
    QObject::connect(HTTPQuery, SIGNAL(ErrorOccurred()), this, SLOT(onHTTPError()));
}

TSystemInfo::~TSystemInfo()
{
    Config->deleteLater();

    SendLogMsg(MSG_CODE::CODE_OK, "Successfully finished");
    DB.close();
}

void TSystemInfo::SendLogMsg(uint16_t Category, const QString &Msg)
{
    if (DebugMode) {
        qDebug() << Msg;
    }
    QSqlQuery QueryLog(DB);
    DB.transaction();
    QString QueryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(Category) + ", "
                        "\'SystemInfo\', "
                        "\'" + Msg +"\'"
                        ")";

    if (!QueryLog.exec(QueryText)) {
        qDebug() << "FAIL Cannot execute query. Error: " << QueryLog.lastError().text() << " Query: "<< QueryLog.lastQuery();
        DB.rollback();
        exit(-1);
    }
    if (!DB.commit()) {
        qDebug() << "FAIL Cannot commit transation. Error: " << DB.lastError().text();
        DB.rollback();
        exit(-2);
    };
}

void TSystemInfo::onSendLogMsg(uint16_t Category, const QString &Msg)
{
    SendLogMsg(Category, Msg);
}

void TSystemInfo::onStartGetData()
{
//    qDebug() << "timeout updatetimer";
    if (GettingInformation) {
        return;
    }

    TimeOfRun = QTime::currentTime();
    if (DebugMode)  {
        qDebug() << "Start get information about systems. Time:" << TimeOfRun.msecsTo(QTime::currentTime()) << "ms" ;
    }

    GettingInformation = true;
    AboutSystem->UpdataAboutSystem();
}

void TSystemInfo::onGetDataComplite()
{
    if (DebugMode)  {
        qDebug() << "System data received successfully. Save to DB. Time:" << TimeOfRun.msecsTo(QTime::currentTime()) << "ms" ;
    }

    QSqlQuery QueryAdd(DB);
    DB.transaction();

    while(!QueueSaveToDB.isEmpty()) {
        TSaveToDBItem tmp = QueueSaveToDB.dequeue();
        QString QueryText = "INSERT INTO \"PARAMS\" (\"DATE_TIME\", \"SENDER\", \"CATEGORY\", \"NAME\", \"NUMBER\", \"CURRENT_VALUE\") VALUES ("
                            "'" + tmp.DateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                            "'" + tmp.Sender + "', " +
                            "'" + tmp.Category + "', " +
                            "'" + tmp.Name + "', " +
                            QString::number(tmp.Number) + ", " +
                            "'" + tmp.Value.toUtf8().toBase64() + "')";
       // qDebug() << QueryText;
        if (!QueryAdd.exec(QueryText)) {
             DB.rollback();
            qDebug() << "Cannot execute query. Error: " + QueryAdd.lastError().text() + " Query: " + QueryAdd.executedQuery();
            exit(-2);
        }
    }
    if (!DB.commit()) {
        DB.rollback();
        qDebug() << "Cannot commit transation. Error: " + DB.lastError().text();
        exit(-4);
    };

    GettingInformation = false;

    SendToHTTPServer();
}

void TSystemInfo::onHTTPGetAnswer(const QByteArray &Answer)
{
    if (DebugMode)  {
        qDebug() << "Received a response from the server. Time:" << TimeOfRun.msecsTo(QTime::currentTime()) << "ms" ;
    }

    if (Answer.left(2) == "OK") {
         QSqlQuery Query(DB);
         DB.transaction();
         for (auto Item : HTTPServerInfo.DeleteID) {
             QString QueryText = "DELETE FROM PARAMS "
                                 "WHERE ID = " + QString::number(Item);
             if (!Query.exec(QueryText)) {
                 DB.rollback();
                 qDebug() << "Cannot execute query. Error: " + Query.lastError().text() + " Query: " + QueryText;
                 exit(-2);
             }
         }


         if (!DB.commit()) {
            DB.rollback();
            qDebug() << "Cannot commit transation. Error: " + DB.lastError().text();
            exit(-4);
         };

         HTTPServerInfo.DeleteID.clear();

         SendLogMsg(TSystemInfo::CODE_INFORMATION, "Data has been successfully sent to the server."
             " LastID: " + QString::number(HTTPServerInfo.CurrentLastID));

         HTTPServerInfo.LastID = HTTPServerInfo.CurrentLastID;

         Config->beginGroup("SERVER");
         Config->setValue("LastID", HTTPServerInfo.LastID);
         Config->endGroup();
    }
    else {
        SendLogMsg(MSG_CODE::CODE_ERROR, "Failed to send data to the server. Server answer: " + Answer);
    }

    Sending = false;
}

void TSystemInfo::onHTTPError()
{
    Sending = false;
    if (DebugMode) {
        qDebug() << "Error getting anwser from HTTP Server. Retry. Time:" << TimeOfRun.msecsTo(QTime::currentTime()) << "ms";
    }

    if (!XMLStr.isEmpty()) HTTPQuery->Run(XMLStr);
}

void TSystemInfo::SendToHTTPServer()
{
    if (Sending) {
        SendLogMsg(MSG_CODE::CODE_INFORMATION, "Previous request has not been processed yet. Skipped.");
        return;
    }

    QSqlQuery Query(DB);
    DB.transaction();

    QString QueryText = "SELECT FIRST " + QString::number(HTTPServerInfo.MaxRecord) + " " +
                        "ID, DATE_TIME, SENDER, CATEGORY, NAME, NUMBER, CURRENT_VALUE "
                        "FROM PARAMS "
                        "WHERE ID > " + QString::number(HTTPServerInfo.LastID) + " "
                        "ORDER BY ID";


    if (!Query.exec(QueryText)) {
        DB.rollback();
        qDebug() << "Cannot execute query. Error: " + Query.lastError().text() + " Query: " + QueryText;
        exit(-2);
    }
    //форматируем XML документ
    XMLStr.clear();
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", HTTPServerInfo.AZSCode);
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());
    while (Query.next()) {
        XMLWriter.writeStartElement("SystemInfo");
        XMLWriter.writeTextElement("DateTime", Query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("Sender", Query.value("SENDER").toString());
        XMLWriter.writeTextElement("Category", Query.value("CATEGORY").toString());
        XMLWriter.writeTextElement("Name", Query.value("NAME").toString());
        XMLWriter.writeTextElement("Number", Query.value("NUMBER").toString());

//        XMLWriter.writeTextElement("CurrentValue", Codec->toUnicode(Query.value("CURRENT_VALUE").toByteArray()));
        XMLWriter.writeTextElement("CurrentValue", Query.value("CURRENT_VALUE").toString());
       // QTextCodec *Codec = QTextCodec::codecForName("Windows-1251");
       // qDebug() << "->" <<  Codec->fromUnicode(Query.value("CURRENT_VALUE").toByteArray());//Codec->toUnicode(Query.value("CURRENT_VALUE").toByteArray());
        XMLWriter.writeEndElement();
        HTTPServerInfo.DeleteID.push_back(Query.value("ID").toLongLong());
    }

    if (Query.last()) HTTPServerInfo.CurrentLastID = Query.value("ID").toUInt();

    XMLWriter.writeEndElement();
    XMLWriter.writeEndDocument();

    if (!DB.commit()) {
       DB.rollback();
       qDebug() << "Cannot commit transation. Error: " + DB.lastError().text();
       exit(-4);
    };

  //  qDebug() << XMLStr;

    if (DebugMode)  {
        qDebug() << "Send data to server. Time:" << TimeOfRun.msecsTo(QTime::currentTime()) << "ms" ;
    }

 /*   QFile file("SI.xml");
    file.open(QIODevice::WriteOnly);
    file.write(XMLStr);
    file.close();*/

    HTTPQuery->Run(XMLStr);
}

void TSystemInfo::onStart()
{
    if (!DB.open()) {
        qCritical() << "Cannot connect to database. Error: " << DB.lastError().text();
        exit(-1);
    };

    SendLogMsg(MSG_CODE::CODE_OK, "Successfully started");

    onStartGetData();

    UpdateTimer.start(); //запускаем таймер обновления данных
}

void TSystemInfo::onSaveToDB(const QString &Sender, const QString &Category, const QString &Name, const uint16_t Number, const QString &Value)
{
    TSaveToDBItem tmp;
    tmp.DateTime = QDateTime::currentDateTime();
    tmp.Sender = Sender;
    tmp.Category = Category;
    tmp.Name = Name;
    tmp.Number = Number;
    tmp.Value = Value;
    QueueSaveToDB.enqueue(tmp);
}
