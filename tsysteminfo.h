//получает общую информацию о ПК
#ifndef TSYSTEMINFO_H
#define TSYSTEMINFO_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QDateTime>
#include <QTimer>
#include <QQueue>
#include "thttpquery.h"
#include "tconsole.h"
#include "taboutsystem.h"

class TSystemInfo : public QObject
{
    Q_OBJECT
public:
    typedef enum {CODE_OK, CODE_ERROR, CODE_INFORMATION} MSG_CODE;
private:
   typedef struct  {
       QString Url;
       QString AZSCode;
       uint16_t MaxRecord;
       uint64_t LastID;
       uint64_t CurrentLastID;
       QVector<uint64_t> DeleteID;
   } THTTPServerInfo;

   typedef struct {
       QDateTime DateTime;
       QString Sender;
       QString Category;
       QString Name;
       uint16_t Number;
       QString Value;
   } TSaveToDBItem;

public:
    explicit TSystemInfo(const QString &ConfigFileName, QObject *parent = nullptr);
    ~TSystemInfo();

private:
    TConsole Console;
    QSettings *Config;
    QSqlDatabase DB;
    QTimer UpdateTimer;
    THTTPQuery *HTTPQuery;
    THTTPServerInfo HTTPServerInfo;
    TAboutSystem *AboutSystem;
    QByteArray XMLStr;
    bool Sending = false; //флаг текущей передачи данных
    bool GettingInformation = false;//флаг получения информации
    bool DebugMode = false;//если истина то выводим отладочную информацию в консоль
    QTime TimeOfRun = QTime::currentTime();

    QQueue<TSaveToDBItem> QueueSaveToDB;

    void SendLogMsg(uint16_t Category, const QString &Msg);
    void SendToHTTPServer();

signals:
    void Finished();

public slots:
    void onStart();
private slots:
    void onSendLogMsg(uint16_t Category, const QString &Msg);//записать сообщение в лог
    void onStartGetData();
    void onGetDataComplite();
    void onHTTPGetAnswer(const QByteArray &Answer);//получен ответ от сервеера
    void onHTTPError(); //ошибка передачи данных на сервер
    void onSaveToDB(const QString &Sender, const QString &Category, const QString &Name, const uint16_t Number, const QString &Value);
};


#endif // TSYSTEMINFO_H
