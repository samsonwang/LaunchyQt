/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "precompiled.h"
#include "Runner.h"
#include "gui.h"
#include "PluginMsg.h"

using namespace launchy;

void Runner::init() {
    cmds.clear();

    if (g_settings->value("runner/version", 0.0).toDouble() == 0.0) {
        g_settings->beginWriteArray("runner/cmds");
        g_settings->setArrayIndex(0);
#ifdef Q_OS_WIN
        g_settings->setValue("name", "cmd");
        g_settings->setValue("file", "C:\\Windows\\System32\\cmd.exe");
        g_settings->setValue("args", "/K $$");
#endif
#ifdef Q_OS_LINUX
        g_settings->setValue("name", "cmd");
        g_settings->setValue("file", "/usr/bin/xterm");
        g_settings->setValue("args", "-hold -e $$");
#endif
        /*
        #ifdef Q_WS_MAC
        set->setValue("name", "cmd");
        set->setValue("file", "")
        #endif
        */
        g_settings->endArray();
    }
    g_settings->setValue("runner/version", 2.0);

    // Read in the array of websites
    int count = g_settings->beginReadArray("runner/cmds");
    for (int i = 0; i < count; ++i) {
        g_settings->setArrayIndex(i);
        runnerCmd cmd;
        cmd.file = g_settings->value("file").toString();
        cmd.name = g_settings->value("name").toString();
        cmd.args = g_settings->value("args").toString();
        cmds.push_back(cmd);
    }
    g_settings->endArray();
}

void Runner::getID(uint* id) {
    *id = HASH_RUNNER;
}

void Runner::getName(QString* str) {
    *str = "Runner";
}

QString Runner::getIcon() {
    return libPath + "/icons/runner.png";
}

QString Runner::getIcon(QString file) {
    Q_UNUSED(file);
#ifdef Q_OS_WIN
    QRegExp rx("\\.(exe|lnk)$", Qt::CaseInsensitive);
    if (rx.indexIn(file) != -1)
        return file;
#endif
    return getIcon();
}


void Runner::getCatalog(QList<CatItem>* items) {
    foreach(runnerCmd cmd, cmds) {
        items->push_back(CatItem(cmd.file + "%%%" + cmd.args,
                                 cmd.name,
                                 HASH_RUNNER,
                                 getIcon(cmd.file)));
    }
}


void Runner::getResults(QList<InputData>* inputData, QList<CatItem>* results)
{
    if (inputData->count() <= 1)
        return;

    CatItem& catItem = inputData->first().getTopResult();
    if (catItem.pluginId == (int)HASH_RUNNER && inputData->last().hasText())
    {
        const QString & text = inputData->last().getText();
        // This is user search text, create an entry for it
        results->push_front(CatItem(text,
                                    text,
                                    HASH_RUNNER,
                                    getIcon(catItem.iconPath)));
    }
}

void Runner::launchItem(QList<InputData>* inputData, CatItem* item)
{
    item = item; // Compiler warning

    QString file = "";
    QString args = "";

    CatItem* base = &inputData->first().getTopResult();

    file = base->fullPath;

    // Replace the $'s with arguments
    QStringList spl = file.split("$$");

    file = spl[0];
    for (int i = 1; i < spl.size(); ++i)
    {
        if (inputData->count() >= i+1)
        {
            //			const InputData* ij = &inputData->at(i);
            CatItem* it = &((InputData)inputData->at(i)).getTopResult();
            file += it->fullPath;
        }
        file += spl[i];
    }

    // Split the command from the arguments
    spl = file.split("%%%");

    file = spl[0];
    if (spl.count() > 0)
        args = spl[1];

    if (file.contains("http://"))
    {
        QUrl url(file);
        file = url.toEncoded();
    }
    //	qDebug() << file << args;
    runProgram(file, args);
}


void Runner::doDialog(QWidget* parent, QWidget** newDlg) {
    if (gui != NULL)
        return;
    gui.reset(new Gui(parent));
    *newDlg = gui.get();
}

void Runner::endDialog(bool accept) {
    if (accept) {
        gui->writeOptions();
        init();
    }
    gui.reset();
}

void Runner::setPath(const QString* path) {
    libPath = *path;
}


Runner::Runner() {
    gui.reset();
    HASH_RUNNER = qHash(QString("runner"));
}

Runner::~Runner() {

}

int Runner::msg(int msgId, void* wParam, void* lParam) {
    bool handled = false;
    switch (msgId) {
    case MSG_INIT:
        init();
        handled = true;
        break;
    case MSG_GET_ID:
        getID((uint*)wParam);
        handled = true;
        break;
    case MSG_GET_NAME:
        getName((QString*)wParam);
        handled = true;
        break;
    case MSG_GET_CATALOG:
        getCatalog((QList<CatItem>*) wParam);
        handled = true;
        break;
    case MSG_GET_RESULTS:
        getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
        handled = true;
        break;
    case MSG_LAUNCH_ITEM:
        launchItem((QList<InputData>*) wParam, (CatItem*)lParam);
        handled = true;
        break;
    case MSG_HAS_DIALOG:
        handled = true;
        break;
    case MSG_DO_DIALOG:
        doDialog((QWidget*)wParam, (QWidget**)lParam);
        break;
    case MSG_END_DIALOG:
        endDialog(wParam != 0);
        break;
    case MSG_PATH:
        setPath((QString*)wParam);

    default:
        break;
    }

    return handled;
}

