/*
PluginPy
Copyright (C) 2018 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PluginWrapper.h"
#include <pybind11/pybind11.h>
#include "PluginMsg.h"
#include "ExportPyPlugin.h"

namespace py = pybind11;

namespace pluginpy {

QMutex PluginWrapper::s_inPythonLock;

PluginWrapper::PluginWrapper(exportpy::Plugin* plugin)
    //ScriptPluginsSynchronizer& scriptPluginsSynchronizer)
    : m_plugin(plugin) {
    //m_scriptPluginsSynchronizer(scriptPluginsSynchronizer)

}

PluginWrapper::~PluginWrapper() {
    m_plugin = nullptr;
}

void PluginWrapper::getID(uint* id) {
    *id = m_plugin->getID();
}

void PluginWrapper::getName(QString* name) {
    std::string pluginName = m_plugin->getName();
    *name = QString::fromStdString(pluginName);
}

void PluginWrapper::init() {
    m_plugin->init();
}

void PluginWrapper::setPath(const QString* path) {
    std::string str = (*path).toStdString();
    m_plugin->setPath(str);
}

void PluginWrapper::getLabels(QList<launchy::InputData>* inputData) {
    std::vector<exportpy::InputData> inputDataList;
    for (auto it = inputData->begin(); it != inputData->end(); ++it) {
        //inputDataList.push_back(exportpy::InputData(&(*it)));
        inputDataList.emplace_back(&(*it));
    }

    m_plugin->getLabels(inputDataList);
}

void PluginWrapper::getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* result) {
    std::vector<exportpy::InputData> inputDataList;
    for (auto it = inputData->begin(); it != inputData->end(); ++it) {
        //inputDataList.push_back(exportpy::InputData(&(*it)));
        inputDataList.emplace_back(&(*it));
    }

    exportpy::CatItemList scriptResults(result);
    m_plugin->getResults(inputDataList, scriptResults);
}

void PluginWrapper::getCatalog(QList<launchy::CatItem>* catItem) {
    exportpy::CatItemList resultList(catItem);
    m_plugin->getCatalog(resultList);
}

void PluginWrapper::launchItem(QList<launchy::InputData>* inputData,
                               launchy::CatItem* item) {
    std::vector<exportpy::InputData> inputDataList;
    for (auto it = inputData->begin(); it != inputData->end(); ++it) {
        inputDataList.emplace_back(&(*it));
    }

    ++item->usage;
    exportpy::CatItem launItem(*item);
    m_plugin->launchItem(inputDataList, launItem);
}

bool PluginWrapper::hasDialog() {
    return m_plugin->hasDialog();
}

void PluginWrapper::doDialog(QWidget* parent, QWidget** newDlg) {
    //m_scriptPluginsSynchronizer.enteringDoDialog();

    void* result = m_plugin->doDialog((void*)parent);

    if (result) {
        *newDlg = static_cast<QWidget*>(result);
    }
    else {
        *newDlg = NULL;
    }
}

void PluginWrapper::endDialog(bool accept) {
    m_plugin->endDialog(accept);
    // m_scriptPluginsSynchronizer.finishedEndDialog();
}

void PluginWrapper::launchyShow() {
    m_plugin->launchyShow();
}

void PluginWrapper::launchyHide() {
    m_plugin->launchyHide();
}

int PluginWrapper::msg(int msgId, void* wParam, void* lParam) {
    if (!m_plugin) {
        qDebug() << "Called PluginWrapper, but it has no script plugin";
        return false;
    }

    /*
    // Handle locks

    // We can't allow Python functions to be called when doDialog was called
    // and endDialog not called yet, since it results in a crash.
    // The following tests for it
    const bool waitingForEndDialog =
        msgId != MSG_END_DIALOG &&
        !m_scriptPluginsSynchronizer.tryLockDialogMutex();

    if (waitingForEndDialog) {
        LOG_DEBUG("A doDialog--endDialog sequence has not ended yet. "
                  "Message id: %i", msgId);
        return false;
    }
    m_scriptPluginsSynchronizer.unlockDialogMutex(); // doDialog will lock it

                                                     // If a python functions is called while another one hasn't returned yet,
                                                     // a crash happens. This is a way to avoid it, not sure it's the best way.
    const bool waitingForPythonFunctionToReturn =
        !m_scriptPluginsSynchronizer.tryLockInPythonMutex();

    if (waitingForPythonFunctionToReturn) {
        LOG_DEBUG("Trying to call a Python function while another one hasn't "
                  "returned yet. Message id: %i", msgId);
        return false;
    }
    */

    // python GIL
//     const bool inPython = !s_inPythonLock.tryLock();
//     if (inPython) {
//         qDebug() << "PluginWrapper::dispatchFunction, wait for python lock"
//             << "msgId:" << msgId;
//         return 0;
//     }

    qDebug() << "PluginWrapper::dispatchFunction, lock mutex, msgId:"
        << msgId;
    // Disptach the actual Python function
    int result = 0;
    try {
        result = dispatchFunction(msgId, wParam, lParam);
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        const char* errInfo = e.what();
        qWarning() << "PluginWrapper::msg, py::error_already_set catched on dispatchFunction,"
            << "msgId:" << msgId << "error info:" << errInfo;
    }
    catch (const std::runtime_error& e) {
        const char* errInfo = e.what();
        qWarning() << "PluginWrapper::msg, std::runtime_error catched on dispatchFunction,"
            << "msgId:" << msgId << "error info:" << errInfo;
    }

    qDebug() << "PluginWrapper::dispatchFunction, unlock mutex, msgId:"
        << msgId;
    //s_inPythonLock.unlock();
    return result;
}

/*
bool PluginWrapper::isInPythonFunction() const {
    const bool waitingForPythonFunctionToReturn =
        !m_scriptPluginsSynchronizer.tryLockInPythonMutex();

    if (waitingForPythonFunctionToReturn) {
        m_scriptPluginsSynchronizer.unlockInPythonMutex();
    }
    return waitingForPythonFunctionToReturn;
    return false;
}
*/

int PluginWrapper::dispatchFunction(int msgId, void* wParam, void* lParam) {
    int handled = false;

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
    case MSG_PATH:
        setPath((const QString*)wParam);
        break;
    case MSG_GET_LABELS:
        getLabels((QList<launchy::InputData>*) wParam);
        handled = true;
        break;
    case MSG_GET_RESULTS:
        getResults((QList<launchy::InputData>*) wParam, (QList<launchy::CatItem>*) lParam);
        handled = true;
        break;
    case MSG_GET_CATALOG:
        getCatalog((QList<launchy::CatItem>*) wParam);
        handled = true;
        break;
    case MSG_LAUNCH_ITEM:
        launchItem((QList<launchy::InputData>*) wParam, (launchy::CatItem*)lParam);
        handled = true;
        break;
    case MSG_HAS_DIALOG:
        // Set to true if you provide a gui
        handled = hasDialog();
        break;
    case MSG_DO_DIALOG:
        // This isn't called unless you return true to MSG_HAS_DIALOG
        doDialog((QWidget*)wParam, (QWidget**)lParam);
        break;
    case MSG_END_DIALOG:
        // This isn't called unless you return true to MSG_HAS_DIALOG
        endDialog((bool)wParam);
        break;
    case MSG_LAUNCHY_SHOW:
        handled = true;
        launchyShow();
        break;
    case MSG_LAUNCHY_HIDE:
        handled = true;
        launchyHide();
        break;
    default:
        break;
    }

    return handled;
}

}
