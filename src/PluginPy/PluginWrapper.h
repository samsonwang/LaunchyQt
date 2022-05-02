
#pragma once

#include <QString>
#include <QList>
#include <QWidget>
#include <QMutex>

#include "LaunchyLib/InputData.h"
#include "LaunchyLib/CatalogItem.h"
#include "LaunchyLib/PluginInterface.h"

namespace exportpy { class Plugin; }

namespace pluginpy {

class PluginWrapper : public launchy::PluginInterface {

public:
    PluginWrapper(exportpy::Plugin* plugin, const QString& pluginName);
    virtual ~PluginWrapper();

public:
    virtual int msg(int msgId, void* wParam = nullptr, void* lParam = nullptr);

private:
    void init();
    
    void getName(QString* name);

    void setPath(const QString* path);
    
    void getLabels(QList<launchy::InputData>* inputData);
    
    void getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* result);
    
    void getCatalog(QList<launchy::CatItem>* catItem);
    
    void launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* catItem);
    
    bool hasDialog();
    
    void doDialog(QWidget* parent, QWidget** dialog);
    
    void endDialog(bool accept);
    
    void launchyShow();
    
    void launchyHide();

    //! Does the actual work of calling a Python function
    int dispatchMsg(int msgId, void* wParam, void* lParam);

private:
    exportpy::Plugin* m_plugin;
    QString m_pluginName;
    static QMutex s_inPythonLock;
};

} // namespace pluginpy
