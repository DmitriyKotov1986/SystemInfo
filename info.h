#ifndef INFO_H
#define INFO_H

#include <QString>
#include <QDateTime>
#include <QDebug>

struct TInfo {
    QString Value;
    bool UpDate = true;
};

struct TPathInfo {
    QString Category;
    QString Name;
    uint32_t Number;
};

QDebug operator << (QDebug QD, const TInfo Info);
bool operator == (const TInfo & Info1, const TInfo & Info2);
bool operator == (const TInfo & Info1, const TInfo & Info2);

QDebug operator << (QDebug QD, const TPathInfo PathInfo);
bool operator == (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2);
bool operator != (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2);
bool operator < (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2);

#endif // INFO_H
