
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

// do not throw std::runtime_error when function is not overrided
#define PYBIND11_OVERLOAD_PURE_NOEXCEPT(ret_type, cname, fn, ...) \
        PYBIND11_OVERLOAD_INT(ret_type, cname, #fn, __VA_ARGS__)

class PluginHelper : public Plugin {
public:
    /* Inherit the constructors */
    using Plugin::Plugin;

    /* Trampoline (need one for each virtual function) */
    void init() override {
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,           /* Return type */
            Plugin,         /* Parent class */
            init            /* Name of function in C++ (must match Python name) */
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
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            setPath,
            path
        );
    }

    void getLabels(const std::vector<InputData>& inputDataList) override {
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            getLabels,
            inputDataList
        );
    }

    void getResults(const std::vector<InputData>& inputDataList,
                    const CatItemList& resultsList) override {
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            getResults,
            inputDataList,
            resultsList
        );
    }

    void getCatalog(const CatItemList& resultsList) override {

        //pybind11::gil_scoped_acquire gil;
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
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
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
            auto pwo = py::handle(pw);
            py::object result;
            auto o = overload(pwo);

            if (py::detail::cast_is_temporary_value_reference<py::object>::value) {
                static py::detail::overload_caster_t<py::object> caster;
                result = py::detail::cast_ref<py::object>(std::move(o), caster);
            }
            else {
                result = py::detail::cast_safe<py::object>(std::move(o));
            }

            PyObject* resultPtr = result.ptr();
            if (resultPtr
                && PyObject_IsInstance(resultPtr, (PyObject*)&PyLong_Type)) {
                return PyLong_AsVoidPtr(resultPtr);
            }
        }

        return NULL;
    }

    void endDialog(bool accept) override {
        //PYBIND11_OVERLOAD_PURE
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            endDialog,
            accept
        );
    }

    void launchyShow() override {
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            launchyShow
        );
    }

    void launchyHide() override {
        PYBIND11_OVERLOAD_PURE_NOEXCEPT(
            void,
            Plugin,
            launchyHide
        );
    }
};

}
