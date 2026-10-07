
#pragma once

#include <string>
#include <vector>

#include <Python.h>

#include <pybind11/pybind11.h>

#include "ExportPyInputData.h"
#include "ExportPyCatItem.h"

namespace py = pybind11;

namespace exportpy {

void ExportPlugin(const pybind11::module& m);

class Plugin {
public:
    Plugin() = default;
    virtual ~Plugin() = default;

public:
    virtual void init() = 0;

    virtual std::string getName() = 0;

    virtual void setPath(const std::string& path) = 0;

    virtual void getLabels(const std::vector<InputData>& inputDataList) = 0;

    virtual void getResults(const std::vector<InputData>& inputDataList,
                            const CatItemList& resultsList) = 0;

    virtual void getCatalog(const CatItemList& resultsList) = 0;

    virtual void launchItem(const std::vector<InputData>& inputDataList,
                            const CatItem& item) = 0;

    virtual void* doDialog(void* parentWidget) = 0;

    virtual void endDialog(bool accept) = 0;

    virtual void launchyShow() = 0;

    virtual void launchyHide() = 0;
};

class PluginHelper : public Plugin {
public:
    /* Inherit the constructors */
    PluginHelper()
        : Plugin() {

    }

    virtual ~PluginHelper() {

    }

    /* Trampoline (need one for each virtual function) */
    void init() override {
        PYBIND11_OVERLOAD_PURE(
            void,           /* Return type */
            Plugin,         /* Parent class */
            init            /* Name of function in C++ (must match Python name) */
        );
    }

    std::string getName() override {
        PYBIND11_OVERLOAD_PURE(
            std::string,
            Plugin,
            getName
        );
    }

    void setPath(const std::string& path) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            setPath,
            path
        );
    }

    void getLabels(const std::vector<InputData>& inputDataList) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getLabels,
            inputDataList
        );
    }

    void getResults(const std::vector<InputData>& inputDataList,
                    const CatItemList& resultsList) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getResults,
            inputDataList,
            resultsList
        );
    }

    void getCatalog(const CatItemList& resultsList) override {

        // This function runs on the catalog builder thread, so it needs the gil
        // like every other call into python. It is only safe now that the main
        // thread lets go of the gil while the event loop runs (see
        // PluginMgr::releaseGilForEventLoop); before that, taking it here simply
        // deadlocked against the main thread and had to be left out.
        py::gil_scoped_acquire gil;

        py::function overload = py::get_overload(static_cast<const Plugin*>(this),
                                                 "getCatalog");
        if (overload) {
            overload(resultsList);
        }

        /*
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getCatalog,
            resultsList
        );
        */
    }

    void launchItem(const std::vector<InputData>& inputDataList,
                    const CatItem& item) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            launchItem,
            inputDataList,
            item
        );
    }

    void* doDialog(void* parentWidget) override {
        py::gil_scoped_acquire gil;
        py::function overload = py::get_overload(static_cast<const Plugin*>(this),
                                                 "doDialog");
        if (overload) {
            PyObject* pw = PyLong_FromVoidPtr(parentWidget);
            py::object result = overload(py::handle(pw));

            // py::object result;
            if (py::detail::cast_is_temporary_value_reference<py::object>::value) {
                static py::detail::override_caster_t<py::object> s_caster;
                result = py::detail::cast_ref<py::object>(std::move(result), s_caster);
            }
            else {
                result = py::detail::cast_safe<py::object>(std::move(result));
            }

            PyObject* resultPtr = result.ptr();
            if (resultPtr
                && PyObject_IsInstance(resultPtr, (PyObject*)&PyLong_Type)) {
                return PyLong_AsVoidPtr(resultPtr);
            }
        }

        return nullptr;
    }

    // endDialog closes the doDialog sequence, so a plugin that opened a dialog
    // has to implement it, but it is looked up without failing like the two
    // notifications below: PYBIND11_OVERLOAD_PURE raises "Tried to call pure
    // virtual function" *after* releasing the gil, and a plugin that leaves it
    // out would then take the whole launcher down on every dialog close.
    void endDialog(bool accept) override {
        py::gil_scoped_acquire gil;
        py::function overload = py::get_override(static_cast<const Plugin*>(this),
                                                 "endDialog");
        if (overload) {
            overload(accept);
        }
    }

    // launchyShow and launchyHide are notifications, and only a few plugins
    // implement them. Going through PYBIND11_OVERLOAD_PURE here made every
    // plugin that does not override them throw "Tried to call pure virtual
    // function" on every show and hide, so they are looked up without failing.
    void launchyShow() override {
        py::gil_scoped_acquire gil;
        py::function overload = py::get_override(static_cast<const Plugin*>(this),
                                                 "launchyShow");
        if (overload) {
            overload();
        }
    }

    void launchyHide() override {
        py::gil_scoped_acquire gil;
        py::function overload = py::get_override(static_cast<const Plugin*>(this),
                                                 "launchyHide");
        if (overload) {
            overload();
        }
    }
};

} // namespace exportpy
