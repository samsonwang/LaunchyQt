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

#pragma once

#include <QString>
#include <QTranslator>

namespace launchy {
class TranslationManager {
public:
    static TranslationManager& instance();

public:
    void setLocale(const QLocale& loc);
    const QLocale& getLocale() const;

    QList<QLocale> getAllLocales();

private:
    TranslationManager();
    Q_DISABLE_COPY(TranslationManager)

private:
    void load();


private:
    QLocale m_loc;
    QTranslator m_translatorQt;
    QTranslator m_translatorLaunchy;
};
}
