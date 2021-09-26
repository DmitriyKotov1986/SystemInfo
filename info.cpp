#include "info.h"

//TInfo operators
QDebug operator << (QDebug QD, const TInfo Info) {
    QString Str = Info.Value + "-" + Info.DateTime.toString(Qt::TextDate) + " UPDATE:" + (Info.UpDate ? "TRUE" : "FALSE");
    QD << Str;
    return QD;
}

bool operator == (const TInfo & Info1, const TInfo & Info2) {
    return Info1.Value == Info2.Value;
}

bool operator != (const TInfo & Info1, const TInfo & Info2) {
    return !(Info1 == Info2);
}


//TPathInfo operators
QDebug operator << (QDebug QD, const TPathInfo PathInfo) {
    QString Str = PathInfo.Category + "/" + PathInfo.Name + "/" + QString::number(PathInfo.Number);
    QD << Str;
    return QD;
}

bool operator == (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2) {
    return (PathInfo1.Category == PathInfo2.Category) && (PathInfo1.Name == PathInfo2.Name) && (PathInfo1.Number == PathInfo2.Number);
}

bool operator != (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2) {
    return !(PathInfo1 == PathInfo2);
}

bool operator < (const TPathInfo & PathInfo1, const TPathInfo & PathInfo2) {
    if (PathInfo1.Category < PathInfo2.Category) return true;
    else if (PathInfo1.Category == PathInfo2.Category) {
        if (PathInfo1.Name < PathInfo2.Name) return true;
        else if (PathInfo1.Name == PathInfo2.Name)   {
            if (PathInfo1.Number < PathInfo2.Number) return true;
        }
    }
    return false;
}
