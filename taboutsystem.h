#ifndef TABOUTSYSTEM_H
#define TABOUTSYSTEM_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <QSettings>
#include <QtSql/QSqlDatabase>
#include <QDateTime>
#include <QProcess>
#include "info.h"

class TAboutSystem : public QObject
{
    Q_OBJECT
public:
    explicit TAboutSystem(QSettings &Config, QObject *parent = nullptr);
    ~TAboutSystem();

private:
    QMap <TPathInfo, TInfo> Info;
    QMap <QString, uint32_t> CurrentNumberKey;
    QSettings &Config;
    QSqlDatabase DB;
    QString WMICFileName;
    QStringList GroupList;
    uint16_t CurrentGroupIndex = 0;
    QProcess *cmd;
    QString CodePage;

    void Parser(const QString& Group, const QByteArray& str);
    uint32_t NextKey(const QString & OldKey);
    void Updata();
    void NextUpdate();

public:
    void UpdataAboutSystem();

signals:
    void GetDataComplite();
    void SaveToDB(const QString &Sender, const QString &Category, const QString &Name,  const uint16_t Number, const QString &Value);
    void SendLogMsg(uint16_t Category, const QString &Msg);

private slots:
    void onCmdFinished(int exitCode, QProcess::ExitStatus exitStatus = QProcess::NormalExit);
    void onErrorOccurred(QProcess::ProcessError error);
};

#endif // TABOUTSYSTEM_H
