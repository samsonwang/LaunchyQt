
#pragma once

#include "PluginInterface.h"
#include "ExportPyPlugin.h"
#include "InputData.h"
#include "CatalogItem.h"

namespace pluginpy {

class PluginWrapper : public launchy::PluginInterface {

public:
    PluginWrapper(exportpy::Plugin* plugin);
    virtual ~PluginWrapper();

public:
    virtual int msg(int msgId, void* wParam = nullptr, void* lParam = nullptr);

private:
    void init();
    void setPath(const QString* path);
    void getLabels(QList<launchy::InputData>* inputData);
    void getID(uint* id);
    void getName(QString* name);
    void getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* result);
    void getCatalog(QList<launchy::CatItem>* catItem);
    void launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* catItem);
    bool hasDialog();
    void doDialog(QWidget* parent, QWidget** dialog);
    void endDialog(bool accept);
    void launchyShow();
    void launchyHide();

    //! Does the actual work of calling a Python function
    int dispatchFunction(int msgId, void* wParam, void* lParam);

private:
    exportpy::Plugin* m_plugin;
    static QMutex s_inPythonLock;
};

}
