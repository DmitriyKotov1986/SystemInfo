//получает общую информацию о ПК
#ifndef TSYSTEMINFO_H
#define TSYSTEMINFO_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QSettings>
#include <QtSql/QSqlDatabase>

class TSystemInfo : public QObject
{
    Q_OBJECT

private:
    enum MSG_CODE {CODE_OK, CODE_ERROR};
    QMap <QString, QString> Info;
    QMap <QString, uint32_t> CurrentNumberKey;

    QSettings Config;
    QSqlDatabase DB;

    void Parser(const QString &Group, const QByteArray &str);
    QString NextKey(const QString & OldKey);
    void SendLogMsg(uint16_t Code, const QString &Msg);



public:
    explicit TSystemInfo(const QString &FileName);
    ~TSystemInfo();

    void Updata();
    void SaveToDB();

    void Print();

public slots:
    void StartGetInformation();

signals:
    void GetInformationComplite();
};


#endif // TSYSTEMINFO_H
