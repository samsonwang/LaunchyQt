
#pragma once

#include "PluginInterface.h"
#include "ExportPyPlugin.h"

namespace pluginpy {

class PluginWrapper : public launchy::PluginInterface {

public:
    PluginWrapper(exportpy::Plugin* plugin);
                 //ScriptPluginsSynchronizer& scriptPluginsSynchronizer);
    virtual ~PluginWrapper();

    virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL);

private:
    void getLabels(QList<InputData>*);
    void getID(uint*);
    void getName(QString*);
    void getResults(QList<InputData>* id, QList<CatItem>* results);
    void getCatalog(QList<CatItem>* items);
    void launchItem(QList<InputData>*, CatItem*);
    bool hasDialog();
    void doDialog(QWidget* parent, QWidget**);
    void endDialog(bool accept);
    void init();
    QString getIcon();
    void launchyShow();
    void launchyHide();

private:
    exportpy::Plugin* m_plugin;

};

}