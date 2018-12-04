
#pragma once

#include <string>
#include <vector>
#include <pybind11/pybind11.h>
#include "ExportPyInputData.h"
#include "ExportPyCatItem.h"

namespace exportpy {

void ExportPlugin(const pybind11::module& m);

class Plugin {
public:
    Plugin() = default;

    virtual void init() = 0;

    virtual unsigned int getID() = 0;

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
    using Plugin::Plugin;

    /* Trampoline (need one for each virtual function) */
    void init() override {
        PYBIND11_OVERLOAD_PURE(
            void,           /* Return type */
            Plugin,         /* Parent class */
            init            /* Name of function in C++ (must match Python name) */
                            //n_times       /* Argument(s) */
        );
    }

    unsigned int getID() override {
        PYBIND11_OVERLOAD_PURE(
            unsigned int,
            Plugin,
            getID
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

        //pybind11::gil_scoped_acquire gil;
        pybind11::function overload = pybind11::get_overload(static_cast<const Plugin *>(this), "getCatalog");
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
        PyObject* pw = PyLong_FromVoidPtr(parentWidget);
        auto pwo = py::handle(pw);
        py::object result;

        pybind11::gil_scoped_acquire gil;
        pybind11::function overload = pybind11::get_overload(static_cast<const Plugin *>(this), "doDialog");
        if (overload) {
            auto o = overload(pwo);
            if (pybind11::detail::cast_is_temporary_value_reference<py::object>::value) {
                static pybind11::detail::overload_caster_t<py::object> caster;
                result = pybind11::detail::cast_ref<py::object>(std::move(o), caster);
            }
            else {
                result = pybind11::detail::cast_safe<py::object>(std::move(o));
            }
        }
        
        PyObject* resultPtr = result.ptr();
        const bool isLong = PyObject_IsInstance(resultPtr, (PyObject*)&PyLong_Type);
        if (isLong) {
            return PyLong_AsVoidPtr(resultPtr);
        }
        else {
            return NULL;
        }

        /*
        PYBIND11_OVERLOAD_PURE(
            void*,
            Plugin,
            doDialog,
            parentWidget
        );
        */
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

}