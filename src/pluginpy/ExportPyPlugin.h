
#pragma once

#include <string>
#include <vector>
#include <pybind11/pybind11.h>

namespace exportpy {

void ExportPlugin(const pybind11::module& m);

class Plugin {
public:
    Plugin() = default;

    virtual void init() = 0;

    virtual unsigned int getID() = 0;

    virtual std::string getName() = 0;

    virtual void setPath(const std::string& path) = 0;

    virtual void getLabels(std::vector<std::string>& inputDataList) = 0;

    virtual void getResults(std::vector<std::string>& inputDataList,
                            std::vector<std::string>& resultsList) = 0;

    virtual void getCatalog(std::vector<std::string>& resultsList) = 0;

    virtual void launchItem(std::vector<std::string>& inputDataList,
                            std::string& item) = 0;

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

    void getLabels(std::vector<std::string>& inputDataList) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getLabels,
            inputDataList
        );
    }

    void getResults(std::vector<std::string>& inputDataList,
                    std::vector<std::string>& resultsList) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getResults,
            inputDataList,
            resultsList
        );
    }

    void getCatalog(std::vector<std::string>& resultsList) override {
        PYBIND11_OVERLOAD_PURE(
            void,
            Plugin,
            getCatalog,
            resultsList
        );
    }

    void launchItem(std::vector<std::string>& inputDataList,
                    std::string& item) override {
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
        PYBIND11_OVERLOAD_PURE(
            void*,
            Plugin,
            doDialog,
            parentWidget
        );
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