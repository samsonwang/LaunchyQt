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

#pragma once

#include <string>

namespace launchy { class InputData; }

namespace exportpy {

class CatItem;

class InputData {
public:
    InputData(launchy::InputData* data);

    /** Apply a label to this query segment */
    void setLabel(const std::string& label);
    /** Remove a label from this query segment */
    void removeLabel(const std::string& label);
    /** Check if it has the given label applied to it */
    bool hasLabel(const std::string& label);

    /** Set the id of this query
    This can be used to override the owner of the selected catalog item, so that
    no matter what item is chosen from the catalog, the given plugin will be the one
    to execute it.
    \param i The plugin id of the plugin to execute the query's best match from the catalog
    */
    void setPlugin(const std::string& plugin);

    /** Returns the current owner id of the query */
    std::string getPlugin() const;

    /** Get the text of the query segment */
    std::string getText() const;

    /** Set the text of the query segment */
    void setText(const std::string& text);

    /** Get the text of the query segment */
    bool hasText() const;

    /** Get a pointer to the best catalog match for this segment of the query */
    CatItem getTopResult();

private:
    launchy::InputData* m_data;
};

} // namespace exportpy
