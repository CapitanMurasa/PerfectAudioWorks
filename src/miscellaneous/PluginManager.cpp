#include "PluginManager.h"
#include <filesystem>
#include <iostream>
#include <QDebug> 

extern PortaudioThread* global_audiothread;

namespace fs = std::filesystem;
namespace py = pybind11;

PluginManager::PluginManager(QObject* parent) : QObject(parent) {
    if (global_audiothread) {

        connect(global_audiothread, &PortaudioThread::playbackProgress, this, &PluginManager::OnTick);
    }
}

void PluginManager::LoadPlugin(const std::string& filePath) {
    namespace fs = std::filesystem;
    fs::path path(filePath);

    if (!fs::exists(path) || path.extension() != ".py") return;

    try {
        std::string dir = path.parent_path().string();
        std::string moduleName = path.stem().string();


        py::module_ sys = py::module_::import("sys");
        bool inPath = false;
        for (auto p : sys.attr("path")) {
            if (p.cast<std::string>() == dir) { inPath = true; break; }
        }
        if (!inPath) sys.attr("path").attr("append")(dir);

        py::module_ newPlugin = py::module_::import(moduleName.c_str());
        py::module_::import("importlib").attr("reload")(newPlugin);

        loaded_plugins.push_back(newPlugin);

        if (py::hasattr(newPlugin, "on_load")) {
            newPlugin.attr("on_load")();
        }

        qDebug() << "Loaded Plugin:" << QString::fromStdString(moduleName);
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to load plugin:" << e.what();
    }
}

void PluginManager::OnTick() {
    if (!global_audiothread) return;

    unsigned long long current_frames = global_audiothread->getCurrentFrame();
    int sample_rate = global_audiothread->getSampleRate(); 

    double current_seconds = (sample_rate > 0) ?
        (double)current_frames / (double)sample_rate : 0.0;

    for (auto& plugin : loaded_plugins) {
        try {
            if (py::hasattr(plugin, "on_tick")) {
                plugin.attr("on_tick")(current_seconds);
            }
        }
        catch (const std::exception& e) {
            qWarning() << "Plugin Tick Error:" << e.what();
        }
    }
}

void PluginManager::OnTrackChanged(const std::string& title, const std::string& artist) {
    for (auto& plugin : loaded_plugins) {
        try {
            if (py::hasattr(plugin, "on_track_change")) {
                plugin.attr("on_track_change")(title, artist);
            }
        }
        catch (const std::exception& e) {
            qCritical() << "Plugin Error during TrackChange:" << e.what();
        }
    }
}