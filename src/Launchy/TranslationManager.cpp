/*
LaunchyQt
Copyright (C) 2019 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "TranslationManager.h"

#include <QApplication>
#include <QDebug>
#include <QDir>

namespace launchy {

TranslationManager& TranslationManager::instance() {
    static TranslationManager s_obj;
    return s_obj;
}

void TranslationManager::setLocale(const QLocale& loc) {
    qInfo() << "LanguageManager::setLang, set language to" << loc;
    m_loc = loc;

    if (m_loc == QLocale("en")) {
        qApp->removeTranslator(&m_translatorLaunchy);
        qApp->removeTranslator(&m_translatorQt);
    }
    else if (m_translatorLaunchy.load(loc,
                                 QString("launchy"),
                                 QString("_"),
                                 QString("translations"))) {

        qApp->installTranslator(&m_translatorLaunchy);

        if (m_translatorQt.load(loc,
                                QString("qt"),
                                QString("_"),
                                QString("translations"))) {
            qApp->installTranslator(&m_translatorQt);
        }
    }
}

const QLocale& TranslationManager::getLocale() const {
    return m_loc;
}

QList<QLocale> TranslationManager::getAllLocales() {
    QList<QLocale> locales;

    QString transPath = qApp->applicationDirPath() + "/translations";
    QStringList trans = QDir(transPath).entryList(QStringList("launchy_*.qm"), QDir::Files);

    for (int i = 0; i< trans.size(); ++i) {
        QString lang = trans.at(i);
        // get locale name from file name
        lang = lang.mid(8);
        lang.chop(3); // remove file extension

        QLocale loc(lang);
        if (!loc.nativeLanguageName().isEmpty()) {
            locales.push_back(loc);
        }
    }

    return locales;
}

TranslationManager::TranslationManager() {

}

}