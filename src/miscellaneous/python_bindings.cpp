#include <pybind11/pybind11.h>
#include <pybind11/embed.h> 
#include "python_bindings.h"
#include <QString> 
#include <QDebug> 
#include <QMetaObject> 

#include "../PAW_GUI/main_paw_widget.h"
#include "../AudioPharser/PortAudioHandler.h"

#include "PythonEvent.h"

namespace py = pybind11;

PythonEventThread* global_pyevent = nullptr;

class PAW_Interface {
public:
    PAW_Interface() {
        
    }

    bool is_ready() const {
        return global_paw_widget != nullptr;
    }

    void setInfo(std::string pluginname) {
        if (global_pyevent) {
            QMetaObject::invokeMethod(global_pyevent, "InitializePlugin",
                Qt::QueuedConnection,
                Q_ARG(QString, QString::fromStdString(pluginname)));
        }
    }

    void playFile(std::string filename) {
        if (is_ready()) {
            QMetaObject::invokeMethod(global_paw_widget, [filename]() {
                global_paw_widget->StopPlayback();
                global_paw_widget->start_playback(QString::fromStdString(filename));
                });
        }
    }

    void sendmessage(std::string message) {
        if (global_pyevent) {
            QMetaObject::invokeMethod(global_pyevent, "sendMessagebox",
                Qt::QueuedConnection,
                Q_ARG(Messagetype, PAW_INFO),
                Q_ARG(QString, QString::fromStdString(message)));
        }
    }

    void registerUpdate(py::function callback, int interval_ms) {
        if (global_pyevent) {
            global_pyevent->registerCallback(callback, interval_ms);
        }
        else {
            qWarning() << "Cannot register callback: global_pyevent is null";
        }
    }

    bool isPlaybackActive() {
        if (global_audiothread) return global_audiothread->m_isRunning;
        return false;
    }

    std::string getTitle() {
        return is_ready() ? global_paw_widget->file_info_current.title : "";
    }

    std::string getArtist() {
        return is_ready() ? global_paw_widget->file_info_current.artist : "";
    }

    std::string getAlbum() {
        return is_ready() ? global_paw_widget->file_info_current.album : "";
    }

    std::string getGenre() {
        return is_ready() ? global_paw_widget->file_info_current.genre : "";
    }

    std::string getTimeElapsed() {
        return is_ready() ? global_paw_widget->returnTimeElapsed().toStdString() : "00:00";
    }

    std::string getTotalTime() {
        return is_ready() ? global_paw_widget->returnTimeStamp().toStdString() : "00:00";
    }
};

PYBIND11_EMBEDDED_MODULE(PAW_python, m) {
    m.doc() = "PerfectAudioWorks API";

    py::class_<PAW_Interface>(m, "PAW")
        .def(py::init<>()) 
        .def("play", &PAW_Interface::playFile)
        .def("is_ready", &PAW_Interface::is_ready)
        .def("setInfo", &PAW_Interface::setInfo)
        .def("SendMessageBox", &PAW_Interface::sendmessage)
        .def("register_update", &PAW_Interface::registerUpdate)
        .def("IsPlaybackActive", &PAW_Interface::isPlaybackActive)
        .def("GetTitle", &PAW_Interface::getTitle)
        .def("GetArtist", &PAW_Interface::getArtist)
        .def("GetAlbum", &PAW_Interface::getAlbum)
        .def("GetGenre", &PAW_Interface::getGenre)
        .def("GetTimeElapsed", &PAW_Interface::getTimeElapsed)
        .def("GetTotalTime", &PAW_Interface::getTotalTime);
}