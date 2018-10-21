#pragma once
#include <QtGui>


#include <QFileIconProvider>

#include <QWidget>


class UnixIconProvider : public QFileIconProvider
{
 private:
    QHash<QString, QString> file2mime;
    QHash<QString, QString> mime2desktop;
    QHash<QString, QString> desktop2icon;
    QHash<QString, QString> icon2path;
    QStringList xdgDataDirs;
 public:
    UnixIconProvider();
    ~UnixIconProvider() {}
    virtual QIcon icon(const QFileInfo& info);
    QString getDesktopIcon(QString desktopFile, QString IconName = "");
};

