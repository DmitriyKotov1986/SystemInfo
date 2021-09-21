//получает общую информацию о ПК
#ifndef TSYSTEMINFO_H
#define TSYSTEMINFO_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QSettings>
#include <QtSql/QSql>
#include <QtSql/QSqlDatabase>


class TSystemInfo
{
private:
    QMap <QString, QString> Info;
    QMap <QString, uint32_t> CurrentNumberKey;
    void Parser(const QString &Group, const QByteArray &str);
    QString NextKey(const QString & OldKey);

    QSettings Config;

    QSqlDatabase DB;

public:
    TSystemInfo(const QString &FileName);
    ~TSystemInfo();

    void Updata();
    void SaveToDB();

    void Print();
};

#endif // TSYSTEMINFO_H
