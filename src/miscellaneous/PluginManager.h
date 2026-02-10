#pragma once
#include <QObject>
#include <vector>
#include <string>

#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#pragma pop_macro("slots")

#include "../AudioPharser/PortAudioHandler.h"

namespace py = pybind11;

class PluginManager : public QObject {
    Q_OBJECT

public:
    explicit PluginManager(QObject* parent = nullptr);

    void LoadPlugin(const std::string& filePath);
    //void LoadPluginsFromFolder(const std::string& folderPath);

public slots:
    void OnTick();
    void OnTrackChanged(const std::string& title, const std::string& artist);

private:

    std::vector<py::module_> loaded_plugins;
};