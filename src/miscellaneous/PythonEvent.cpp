#include "PythonEvent.h"

#include <QTimer>
#include <QFileInfo>
#include <QMessageBox>

namespace py = pybind11;

PythonEventThread::PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget){

}

PythonEventThread::~PythonEventThread(){
    
}


void PythonEventThread::loadPluginAsync(const QString& filePath) {

    bool success = openPluginInternal(filePath);

    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    emit pluginLoadFinished(success, filePath, fileName);
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

        bool inPath = false;
        for (auto p : sys.attr("path")) {
            if (p.cast<std::string>() == stdDirPath) { inPath = true; break; }
        }
        if (!inPath) sys.attr("path").attr("append")(stdDirPath);

        py::module_ mod = py::module_::import(moduleName.toStdString().c_str());
        py::module_ importlib = py::module_::import("importlib");
        importlib.attr("reload")(mod);

        qDebug() << "Loaded Plugin:" << moduleName;
        return true;

    }
    catch (const std::exception& e) {
        qCritical() << "Python Plugin Error:" << e.what();
        return false;
    }
}