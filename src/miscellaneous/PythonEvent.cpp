#include "PythonEvent.h"

#include <QTimer>
#include <QFileInfo>
#include <QMessageBox>

namespace py = pybind11;

PythonEventThread::PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget)
    : QObject(nullptr)
{

}

PythonEventThread::~PythonEventThread() {
    py::gil_scoped_acquire acquire;
    for (auto cb : activeCallbacks) {
        cb->timer->stop();
        delete cb->timer;
        delete cb; 
    }
    activeCallbacks.clear();
}

void PythonEventThread::InitializePlugin(const QString& Pluginname) {
    qDebug() << "Plugin Registered:" << Pluginname;
    pluginname = Pluginname;
}

void PythonEventThread::loadPluginAsync(const QString& filePath) {
    bool success = openPluginInternal(filePath);

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    emit pluginLoadFinished(success, filePath, fileName, pluginname);
}

bool PythonEventThread::openPluginInternal(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();
    QString moduleName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();

    try {
        py::gil_scoped_acquire acquire;

        py::module_ sys = py::module_::import("sys");
        std::string stdDirPath = dirPath.toStdString();
        std::string stdModuleName = moduleName.toStdString();

        bool inPath = false;
        for (auto p : sys.attr("path")) {
            if (p.cast<std::string>() == stdDirPath) { inPath = true; break; }
        }
        if (!inPath) sys.attr("path").attr("append")(stdDirPath);

        bool isAlreadyLoaded = sys.attr("modules").contains(stdModuleName);


        py::module_ mod = py::module_::import(stdModuleName.c_str());
        

        if (isAlreadyLoaded) {
            py::module_ importlib = py::module_::import("importlib");
            importlib.attr("reload")(mod);
            qDebug() << "Reloaded Plugin:" << moduleName;
        }
        else {
            qDebug() << "Loaded New Plugin:" << moduleName;
        }

        return true;

    }
    catch (const std::exception& e) {
        qCritical() << "Python Plugin Error:" << e.what();
        return false;
    }
}

void PythonEventThread::registerCallback(py::function callback, int interval_ms) {

    PythonCallback* cb = new PythonCallback();
    cb->func = callback; 
    cb->timer = new QTimer(this);

    connect(cb->timer, &QTimer::timeout, [this, cb]() {
        try {
            py::gil_scoped_acquire acquire;

            cb->func();

        }
        catch (const std::exception& e) {
            qWarning() << "Python Callback Error:" << e.what();

        }
        });

    cb->timer->start(interval_ms);
    activeCallbacks.push_back(cb);

    qDebug() << "Registered Python Callback with interval:" << interval_ms << "ms";
}