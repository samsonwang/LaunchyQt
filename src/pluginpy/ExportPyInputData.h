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

#include <set>
#include <pybind11/pybind11.h>
#include "ExportPyCatItem.h"

namespace launchy { class InputData; }

namespace py = pybind11;

namespace exportpy {

void ExportInputData(const py::module& m);

class InputData {
public:
    InputData();
    InputData(const std::string& str);
    InputData(const launchy::InputData& data);

    /** Get the labels applied to this query segment */
    const std::set<unsigned int>& getLabels() const;
    /** Apply a label to this query segment */
    void setLabel(unsigned int l);
    /** Remove a label from this query segment */
    void removeLabel(unsigned int l);
    /** Check if it has the given label applied to it */
    bool hasLabel(unsigned int l);

    /** Set the id of this query

    This can be used to override the owner of the selected catalog item, so that
    no matter what item is chosen from the catalog, the given plugin will be the one
    to execute it.

    \param i The plugin id of the plugin to execute the query's best match from the catalog
    */
    void setID(unsigned int i);

    /** Returns the current owner id of the query */
    unsigned int getID() const;

    /** Get the text of the query segment */
    const std::string& getText() const;

    /** Set the text of the query segment */
    void setText(const std::string& text);

    /** Get the text of the query segment */
    bool hasText() const;

    /** Get a pointer to the best catalog match for this segment of the query */
    CatItem& getTopResult();
    //const CatItem& getTopResult() const;

    /** Change the best catalog match for this segment */
    void setTopResult(const CatItem& sr);

private:
    /** The user's entry */
    std::string m_text;
    /** Any assigned labels to this query segment */
    std::set<unsigned int> m_labels;
    /** A pointer to the best catalog match for this segment of the query */
    CatItem m_topResult;
    /** The plugin id of this query's owner */
    unsigned int m_id;
};
}

