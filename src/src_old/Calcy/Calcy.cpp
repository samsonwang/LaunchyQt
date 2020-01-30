/*
  Launchy: Application Launcher
  Copyright (C) 2007-2010 Josh Karlin

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
#include "LaunchyLib.h"
#include "calcy.h"
#include "gui.h"
#include "PluginMsg.h"
#include "Calculator.h"
#include "Converter.h"

Calcy* g_plugin;

Calcy::Calcy() {
    g_plugin = this;
    HASH_CALCY = qHash(QString("calcy"));
    qDebug() << "Calcy pluginId:" << HASH_CALCY;
}

Calcy::~Calcy() {
    if (m_gui) {
        delete m_gui;
        m_gui = nullptr;
    }
}

void Calcy::init() {
    if (launchy::g_settings.isNull()) {
        return;
    }

    bool useCommaForDecimal = launchy::g_settings->value("calcy/useCommaForDecimal", false).toBool();

    QString decimal = useCommaForDecimal ? "," : ".";
    QString group = useCommaForDecimal ? "." : ",";

    QString pattern = QString("^[\\(\\+\\-]*([\\d\\%1]+(\\%2\\d+)?)").arg(group).arg(decimal);
    qDebug() << "Calcy::init, pattern:" << pattern;

    m_reg.setPattern(pattern);
}

void Calcy::getID(uint* id) {
    *id = HASH_CALCY;
}

void Calcy::getName(QString* str) {
    *str = "Calcy";
}

void Calcy::getLabels(QList<launchy::InputData>* inputList) {
    if (inputList->count() > 1) {
        return;
    }

    QString text = inputList->last().getText();
    text.replace(" ", "");
    if (m_reg.indexIn(text) == 0) {
        qDebug() << "Calcy::getLabels, set last label to HASH_CALCY, matched text:" << text;
        inputList->last().setLabel(HASH_CALCY);
    }
}

void Calcy::getResults(QList<launchy::InputData>* inputList, QList<launchy::CatItem>* results) {
    if (!inputList
        || inputList->isEmpty()
        || !inputList->last().hasLabel(HASH_CALCY)) {
        return;
    }

    QString text = inputList->last().getText();
    qDebug() << "Calcy::getResults, input text:" << text;

    // convert
    if (text.endsWith("=")) {
        qDebug() << "Calcy::getResults, convert";
        text.chop(1);
        bool toNumOk;
        qlonglong num = text.toLongLong(&toNumOk, 0);
        if (!toNumOk) {
            qDebug() << "Calcy::getResults, text to num failed";
            return;
        }
        qDebug() << "Calcy::getResults, num:" << num;
        QString result;
        if (Converter::decStr(num, result)) {
            results->push_back(launchy::CatItem(result + ".dec.calcy", result, HASH_CALCY, getIcon()));
        }
        if (Converter::hexStr(num, result)) {
            results->push_back(launchy::CatItem(result + ".hex.calcy", result, HASH_CALCY, getIcon()));
        }
        if (Converter::octStr(num, result)) {
            results->push_back(launchy::CatItem(result + ".oct.calcy", result, HASH_CALCY, getIcon()));
        }
        if (Converter::binStr(num, result)) {
            results->push_back(launchy::CatItem(result + ".bin.calcy", result, HASH_CALCY, getIcon()));
        }
    }
    // calculate
    else {
        qDebug() << "Calcy::getResults, calculate";

        bool useCommaForDecimal = launchy::g_settings->value("calcy/useCommaForDecimal", false).toBool();
        int outputRounding = launchy::g_settings->value("calcy/outputRounding", 10).toInt();

        QString decimal = useCommaForDecimal ? "," : ".";
        QString group = useCommaForDecimal ? "." : ",";
        text.replace(group, "");
        text.replace(decimal, ".");

        std::string str = text.toStdString();
        qDebug() << "Calcy::getResults, input text(std::string):" << str.c_str();

        double res = 0.0;
        if (!Calculator::calculate(str, res)) {
            return;
        }

        qDebug() << "Calcy::getResults, result:" << res;

        QLocale locale = useCommaForDecimal ? QLocale(QLocale::German) : QLocale(QLocale::C);
        QString resStr = locale.toString(res, 'f', outputRounding);

        // Remove any trailing fractional zeros
        if (resStr.contains(decimal)) {
            while (resStr.endsWith("0")) {
                resStr.chop(1);
            }
            if (resStr.endsWith(decimal)) {
                resStr.chop(1);
            }
        }
        results->push_front(launchy::CatItem(resStr + ".calcy", resStr, HASH_CALCY, getIcon()));
    }
}


int Calcy::launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* item) {
    Q_UNUSED(inputData)
    if (!launchy::g_settings.isNull()
        && launchy::g_settings->value("calcy/copyToClipboard", true).toBool()) {
        QClipboard* clipboard = QApplication::clipboard();
        clipboard->setText(item->shortName);
    }
    return 0;
}

QString Calcy::getIcon() {
    return m_libPath + "/calcy.png";
}

void Calcy::setPath(const QString* path) {
    qDebug() << "Calcy::setPath, path:" << *path;
    m_libPath = *path;
}

void Calcy::doDialog(QWidget* parent, QWidget** newDlg) {
    if (!m_gui.isNull()) {
        m_gui->close();
        delete m_gui;
        m_gui = nullptr;
    }

    m_gui = new Gui(parent);
    *newDlg = m_gui.data();
    init();
}

void Calcy::endDialog(bool accept) {
    if (accept && !m_gui.isNull()) {
        m_gui->writeOptions();
        init();
    }
    m_gui->close();
    delete m_gui;
    m_gui = nullptr;
}

int Calcy::msg(int msgId, void* wParam, void* lParam) {
    int handled = false;
    switch (msgId) {
    case MSG_INIT:
        init();
        handled = true;
        break;
    case MSG_GET_LABELS:
        getLabels((QList<launchy::InputData>*)wParam);
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
    case MSG_GET_RESULTS:
        getResults((QList<launchy::InputData>*) wParam, (QList<launchy::CatItem>*)lParam);
        handled = true;
        break;
    case MSG_LAUNCH_ITEM:
        handled = launchItem((QList<launchy::InputData>*) wParam, (launchy::CatItem*)lParam);
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
        setPath((const QString*)wParam);
    default:
        break;
    }

    return handled;
}
