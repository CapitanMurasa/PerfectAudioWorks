#include "PythonEvent.h"
#include <QTimer>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

namespace py = pybind11;

PythonEventThread::PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget)
    : QObject(nullptr), m_currentLoadingPath("")
{
}

PythonEventThread::~PythonEventThread() {
    clearAllCallbacks();
}

void PythonEventThread::clearAllCallbacks() {
    py::gil_scoped_acquire acquire;
    for (auto cb : activeCallbacks) {
        cb->timer->stop();
        delete cb->timer;
        delete cb;
    }
    activeCallbacks.clear();
}

void PythonEventThread::unloadPlugin(const QString& filePath) {
    py::gil_scoped_acquire acquire;

    auto it = activeCallbacks.begin();
    while (it != activeCallbacks.end()) {
        if ((*it)->ownerPath == filePath) {
            (*it)->timer->stop();
            delete (*it)->timer;
            delete* it;
            it = activeCallbacks.erase(it);
        }
        else {
            ++it;
        }
    }


    py::module_ sys = py::module_::import("sys");
    std::string modName = QFileInfo(filePath).completeBaseName().toStdString();
    if (sys.attr("modules").contains(modName)) {
        sys.attr("modules").attr("pop")(modName);
    }

    py::module_ gc = py::module_::import("gc");
    gc.attr("collect")();
}

void PythonEventThread::loadPluginAsync(const QString& filePath) {
    bool success = openPluginInternal(filePath);
    QString fileName = QFileInfo(filePath).fileName();
    emit pluginLoadFinished(success, filePath, fileName, m_pluginName);
}

void PythonEventThread::InitializePlugin(const QString& name) {
    qDebug() << "Plugin context initialized for:" << name;
    m_pluginName = name;
}

bool PythonEventThread::openPluginInternal(const QString& filePath) {
    m_pluginName = "";
    m_currentLoadingPath = filePath; 

    QFileInfo fileInfo(filePath);
    std::string stdModuleName = fileInfo.completeBaseName().toStdString();
    std::string stdDirPath = fileInfo.absolutePath().toStdString();

    try {
        py::gil_scoped_acquire acquire;
        py::module_ sys = py::module_::import("sys");

        py::list path = sys.attr("path");
        bool exists = false;
        for (auto p : path) { if (p.cast<std::string>() == stdDirPath) { exists = true; break; } }
        if (!exists) path.attr("append")(stdDirPath);

        if (sys.attr("modules").contains(stdModuleName)) {
            py::module_ mod = py::module_::import(stdModuleName.c_str());
            py::module_::import("importlib").attr("reload")(mod);
        }
        else {
            py::module_::import(stdModuleName.c_str());
        }

        m_currentLoadingPath = "";
        return true;
    }
    catch (py::error_already_set& e) {
        emit RequestMessageBox(PAW_ERROR, "Plugin loading error", QString::fromStdString(e.what()));
        m_currentLoadingPath = "";
        return false;
    }
}

void PythonEventThread::registerCallback(py::function callback, int interval_ms) {
    PythonCallback* cb = new PythonCallback();
    cb->func = callback;
    cb->timer = new QTimer(this);
    cb->ownerPath = m_currentLoadingPath; 

    connect(cb->timer, &QTimer::timeout, [cb]() {
        try {
            py::gil_scoped_acquire acquire;
            cb->func();
        }
        catch (const std::exception& e) {
            qWarning() << "Plugin Callback Error:" << e.what();
        }
        });

    cb->timer->start(interval_ms);
    activeCallbacks.push_back(cb);
}

void PythonEventThread::sendMessagebox(Messagetype type, QString title, QString message) {
    emit RequestMessageBox(type, title, message);
}