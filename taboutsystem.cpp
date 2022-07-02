#include <QProcess>
#include "taboutsystem.h"
#include "tsysteminfo.h"

uint32_t TAboutSystem::NextKey(const QString & OldKey) { //генерирует имя для нового ключа
    if (!CurrentNumberKey.contains(OldKey)) {
        CurrentNumberKey.insert(OldKey, 0);
        return 0;
    }
    else {
       ++CurrentNumberKey[OldKey];
       return CurrentNumberKey[OldKey];
    }
}

TAboutSystem::TAboutSystem(QSettings &Config, QObject *parent)
    : QObject(parent)
    , Config(Config)
{
    Config.beginGroup("SYSTEM_INFO");
    //получаем путь к файлу с командой wnim
    WMICFileName = Config.value("wmic", "").toString();
    CodePage = Config.value("CodePage", "IBM 866").toString();
//    qDebug() << "Code page:" << CodePage;
    uint32_t GroupListCount = Config.value("GroupCount", 0).toUInt();
    for (uint32_t i = 0; i < GroupListCount; ++i) GroupList << Config.value("Group" + QString::number(i), "").toString();
    Config.endGroup();
}


void TAboutSystem::Parser(const QString& Group, const QByteArray& str)
{
    QTextStream TS(str);
   // TS.setAutoDetectUnicode(true);
    while (!TS.atEnd()) {
      QString tmp = TS.readLine();
 //     qDebug() << tmp;
      if (tmp != "\r") {
        qsizetype position = tmp.indexOf('=');

        TPathInfo Path;
       // QString Category = Group;
        Path.Category = Group;
        Path.Name = tmp.left(position);
        Path.Number = NextKey(Path.Category + "/" + Path.Name);

        TInfo Value;
        Value.Value = tmp.right(tmp.length() - position - 1);
        Value.Value.chop(1);
        if (Value.Value == "") Value.Value = "n/a";

        if (Info.find(Path) != Info.end()) {
            if ((Info[Path].Value != Value.Value)  || (Path.Category.left(4) == "PING" )) {
                Info[Path].UpDate = true;
                Info[Path].Value = Value.Value;
                //qDebug() << "->" << Path << Info[Path].Value;
            }
        }
        else  {
           // qDebug() << "ADD NEW PATH";
            Value.UpDate = true;
            Info.insert(Path, Value);
        }
      }
    }

   // void SaveToDB(const QString &Sender, const QString &Category, const QString &Name, const uint16_t Number, const QString &Value);

    for (auto &[Key, Value]:Info.toStdMap()) {
        if (Value.UpDate) {
            emit SaveToDB("AboutSystem", Key.Category, Key.Name, Key.Number, Value.Value);
            Value.UpDate = false;
        }
    }

    for (auto &Item : Info) Item.UpDate = false;
    //for (auto &[Key, Value]:Info.toStdMap()) qDebug() << Value.UpDate;
}

TAboutSystem::~TAboutSystem()
{
}

void TAboutSystem::Updata()
{
 //   qDebug() << "Begin:" << GroupList[CurrentGroupIndex];

    Config.beginGroup(GroupList[CurrentGroupIndex]);
    uint32_t Count = Config.value("Count", 0).toUInt();

    //считываем ключи запуска
    QStringList Keys;
    if (GroupList[CurrentGroupIndex].left(4) == "PING") {
        Keys << "path" << "Win32_PingStatus" <<"where" << "address='" + GroupList[CurrentGroupIndex].mid(5, GroupList[CurrentGroupIndex].length() - 5) + "'";
    }
    else Keys << GroupList[CurrentGroupIndex];
    Keys << "GET";
    for (uint32_t i = 0; i < Count; ++i) {
        Keys << Config.value("Key" + QString::number(i), "").toString();
        if (i != Count - 1) Keys << ",";
    }
    Config.endGroup();
    Keys << "/FORMAT:LIST";

    //выполняем команду
    cmd = new QProcess(this);
    QObject::connect(cmd, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onCmdFinished(int, QProcess::ExitStatus)));
    QObject::connect(cmd, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onErrorOccurred(QProcess::ProcessError)));
    cmd->start(WMICFileName ,Keys);
    if (!cmd->waitForStarted(10000)) {
        cmd->kill();
    };
 //   qDebug() << "Start winc: " << WMICFileName << Keys;
}

void TAboutSystem::NextUpdate()
{
    QObject::disconnect(cmd, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onCmdFinished(int, QProcess::ExitStatus)));
    QObject::disconnect(cmd, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onErrorOccurred(QProcess::ProcessError)));
    if (!cmd->waitForFinished(10000)) {
        cmd->kill();
    }

    cmd->deleteLater();

    ++CurrentGroupIndex;
    if (CurrentGroupIndex == GroupList.size()) {
        CurrentGroupIndex = 0;
        emit GetDataComplite();
    }
    else Updata();
}

void TAboutSystem::UpdataAboutSystem()
{
    if (CurrentGroupIndex != 0) return;
    CurrentNumberKey.clear();

    Updata();
}

void TAboutSystem::onCmdFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
 //   qDebug() << "onCmdFinished";
    if ((exitStatus == QProcess::NormalExit)&&(exitCode == 0)) {
        Parser(GroupList[CurrentGroupIndex], cmd->readAllStandardOutput());
    }
    else {
        emit SendLogMsg(TSystemInfo::CODE_ERROR, "An attempt to get data from the system failed. Parameter group:" + GroupList[CurrentGroupIndex]);
    }
    NextUpdate();
}

void TAboutSystem::onErrorOccurred(QProcess::ProcessError error)
{
 //   qDebug() << "onErrorOccurred";
    emit SendLogMsg(TSystemInfo::CODE_ERROR, "An attempt to get data from the system failed. Parameter group:" + GroupList[CurrentGroupIndex]);
    NextUpdate();
}

