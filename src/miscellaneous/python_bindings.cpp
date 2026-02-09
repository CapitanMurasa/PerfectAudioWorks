#include <pybind11/pybind11.h>
#include <pybind11/embed.h> 
#include "python_bindings.h"
#include <QString> 
#include <QDebug> 

#include "file.h" 

namespace py = pybind11;

bool is_ready() {
    return global_paw_widget != nullptr;
}

std::string GetFile() {
    if (!is_ready()) return "";
    return global_paw_widget->m_currentFile.toStdString();
}

bool IsPlaybackActive() {
    if (global_audiothread) {
        return global_audiothread->m_isRunning;
    }
    return false;
}

std::string GetTitle() {
    if (!is_ready()) return "";

    return global_paw_widget->file_info.title;
}

std::string GetArtist() {
    if (!is_ready()) return "";
    return global_paw_widget->file_info.artist;
}

std::string GetAlbum() {
    if (!is_ready()) return "";
    return global_paw_widget->file_info.album;
}

std::string GetGenre() {
    if (!is_ready()) return "";
    return global_paw_widget->file_info.genre;
}


std::string GetTimeElapsed() {
    if (!is_ready()) return "00:00";
    return global_paw_widget->returnTimeElapsed().toStdString();
}

std::string GetTotalTime() {
    if (!is_ready()) return "00:00";
    return global_paw_widget->returnTimeStamp().toStdString();
}


void py_log(std::string message) {
    qDebug().noquote() << "PYTHON:" << QString::fromStdString(message);
}

void PlayFile(std::string filename) {
    if (!is_ready()) return;

    global_paw_widget->StopPlayback();
    global_paw_widget->start_playback(QString::fromStdString(filename));
}


PYBIND11_EMBEDDED_MODULE(PAW_python, m) {
    m.doc() = "PerfectAudioWorks Internal Python API";
    m.def("PlayFile", &PlayFile);
    m.def("IsPlaybackActive", &IsPlaybackActive);
    m.def("GetFile", &GetFile);
    m.def("GetTitle", &GetTitle);
    m.def("GetArtist", &GetArtist);
    m.def("GetAlbum", &GetAlbum);
    m.def("GetGenre", &GetGenre);
    m.def("GetTimeElapsed", &GetTimeElapsed);
    m.def("GetTotalTime", &GetTotalTime);
    m.def("PAWLog", &py_log);
}