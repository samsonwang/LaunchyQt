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
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "StartupOption.h"

#include <QCommandLineParser>
#include <QDebug>

namespace launchy {

StartupOption::StartupOption()
    : rescue(false),
      show(false),
      options(false),
      rescan(false),
      exit(false),
      debug(false),
      multiple(false) {
}

StartupOption parseStartupOption(const QStringList& arguments) {
    QCommandLineParser parser;

    // "-rescue" has to stay one word. The default mode reads a single-dash word
    // as compacted short options, so -rescue would be parsed as -r -e -s -c -u
    // -e and rejected as unknown.
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.addOption(QCommandLineOption("rescue", "reset the skin and the position, then show"));
    parser.addOption(QCommandLineOption("show", "show the launcher"));
    parser.addOption(QCommandLineOption("options", "show the options dialog"));
    parser.addOption(QCommandLineOption("rescan", "rebuild the catalog on start"));
    parser.addOption(QCommandLineOption("exit", "quit instead of running"));
    parser.addOption(QCommandLineOption("debug", "write debug output to launchy.log"));
    parser.addOption(QCommandLineOption("multiple", "start even if another instance is running"));
    parser.addOption(QCommandLineOption("profile", "use the named settings profile", "name"));

    if (!parser.parse(arguments)) {
        qWarning("StartupOption: %s", qPrintable(parser.errorText()));
    }

    StartupOption option;
    option.rescue = parser.isSet("rescue");
    option.show = parser.isSet("show");
    option.options = parser.isSet("options");
    option.rescan = parser.isSet("rescan");
    option.exit = parser.isSet("exit");
    option.debug = parser.isSet("debug");
    option.multiple = parser.isSet("multiple");
    option.profile = parser.value("profile");
    return option;
}

} // namespace launchy
