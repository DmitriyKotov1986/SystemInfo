#include "tsysteminfo.h"
#include <QProcess>
#include <QtDebug>
#include <QSettings>
#include <QTextStream>

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
  // DB.addDatabase(Config);
}

TSystemInfo::~TSystemInfo()
{

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
