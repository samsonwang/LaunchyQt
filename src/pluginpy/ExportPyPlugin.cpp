
#include "ExportPyPlugin.h"

namespace py = pybind11;

namespace exportpy {

void ExportPlugin(const pybind11::module& m) {
    py::class_<Plugin, PluginHelper>(m, "Plugin")
        .def(py::init<>())
        .def("init", &exportpy::Plugin::init)
        .def("getID", &exportpy::Plugin::getID)
        .def("getName", &exportpy::Plugin::getName)
        .def("setPath", &exportpy::Plugin::setPath)
        .def("getLabels", &exportpy::Plugin::getLabels)
        .def("getResults", &exportpy::Plugin::getResults)
        .def("getCatalog", &exportpy::Plugin::getCatalog)
        .def("launchItem", &exportpy::Plugin::launchItem)
        .def("hasDialog", &exportpy::Plugin::hasDialog)
        .def("doDialog", &exportpy::Plugin::doDialog)
        .def("endDialog", &exportpy::Plugin::endDialog)
        .def("launchyShow", &exportpy::Plugin::launchyShow)
        .def("launchyHide", &exportpy::Plugin::launchyHide);
}

}
