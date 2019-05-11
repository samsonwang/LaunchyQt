/*
Launchy
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

#include "TestWidget.h"
#include <QWidget>

namespace py = pybind11;

namespace pluginpy {

TestWidget& TestWidget::instance() {
    static TestWidget s_obj;
    return s_obj;
}

TestWidget::TestWidget()
    : m_testWidget(new QWidget) {

}

void pluginpy::TestWidget::initTestWidget() {

    try {
        // init qsetting
        py::object launchyModule = py::module::import("launchy");
        py::object launchyDict = launchyModule.attr("__dict__");
        PyObject* widgetPyObj = PyLong_FromVoidPtr(m_testWidget);
        launchyDict["__testWidget"] = py::handle(widgetPyObj);

        // run setQSetting from launchy_util
        py::object launchyUtilModule = py::module::import("launchy_test");

        m_testWidget->show();
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        const char* errInfo = e.what();
        qWarning() << "pluginpy::TestWidget::initTestWidget,"
            << "fail to init test widget," << errInfo;
    }
}


}
