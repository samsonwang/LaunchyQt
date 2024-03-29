
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

    virtual bool hasDialog() = 0;

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

        // this function runs in another thread
        // pybind11::gil_scoped_acquire gil;

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

    bool hasDialog() override {
        PYBIND11_OVERLOAD_PURE(
            bool,
            Plugin,
            hasDialog
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

    void endDialog(bool accept) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            endDialog,
            accept
        );
    }

    void launchyShow() override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            launchyShow
        );
    }

    void launchyHide() override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            launchyHide
        );
    }
};

} // namespace exportpy
