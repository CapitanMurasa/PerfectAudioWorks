#include <pybind11/pybind11.h>
#include <pybind11/embed.h> 
#include "python_bindings.h"
#include <QString> 
#include <QDebug> 

namespace py = pybind11;

std::string GetFile() {
    if (!global_paw_widget) {
        return "";
    }
    return global_paw_widget->m_currentFile.toStdString();
}

bool IsPlaybackActive() {
    if (global_audiothread) {
        return global_audiothread->m_isRunning;
    }
    return false;
}

void py_log(std::string message) {
    qDebug().noquote() << "PYTHON:" << QString::fromStdString(message);
}

PYBIND11_EMBEDDED_MODULE(PAW_python, m) {
    m.doc() = "PerfectAudioWorks Internal Python API";
    m.def("GetFile", &GetFile);
    m.def("PAWLog", &py_log);
    m.def("IsPlaybackActive", &IsPlaybackActive);
}