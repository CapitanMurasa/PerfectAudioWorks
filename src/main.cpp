#include <pybind11/embed.h> 
#include "python_bindings.h"

#include "PAW_GUI/main_paw_widget.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include <QLocalServer> 
#include <QLocalSocket>
#include <algorithm>

namespace py = pybind11;

Main_PAW_widget* global_paw_widget = nullptr;
PortaudioThread* global_audiothread = nullptr;

#ifdef Q_OS_LINUX
#include "PAW_GUI/GlobalLinuxKeys.h"
#include <QDBusConnection>
#endif

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setlocale(LC_ALL, ".UTF-8");

    QApplication a(argc, argv);

    const QString serverName = "PerfectAudioWorks_Server_ID";
    const QString lockPath = QDir::temp().absoluteFilePath("perfect_audio_works.lock");

    QLockFile lockFile(lockPath);

    if (!lockFile.tryLock(100)) {
        QLocalSocket socket;
        socket.connectToServer(serverName);

        if (socket.waitForConnected(500)) {
            QStringList args = QCoreApplication::arguments();
            if (args.count() > 1) {
                QByteArray data = args[1].toUtf8();
                socket.write(data);
                socket.waitForBytesWritten(1000);
            }
        }
        else {
            QMessageBox::warning(nullptr, "PAW GUI", "Application is already running, but not responding.");
        }
        return 0;
    }
    
#if WIN32
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring full_path(buffer);

    size_t pos = full_path.find_last_of(L"\\/");
    std::wstring executable_dir = full_path.substr(0, pos);

    std::replace(executable_dir.begin(), executable_dir.end(), L'\\', L'/');

    Py_SetPythonHome(executable_dir.c_str());
#endif

    py::scoped_interpreter guard{};

    QLocalServer::removeServer(serverName);
    QLocalServer server;

    Main_PAW_widget w;

    global_paw_widget = &w;
    global_audiothread = &w.getAudioThread();

    QObject::connect(&server, &QLocalServer::newConnection, [&server, &w]() {
        QLocalSocket* clientConnection = server.nextPendingConnection();

        QObject::connect(clientConnection, &QLocalSocket::disconnected,
            clientConnection, &QLocalSocket::deleteLater);

        QObject::connect(clientConnection, &QLocalSocket::readyRead, [clientConnection, &w]() {
            QByteArray data = clientConnection->readAll();
            QString filePath = QString::fromUtf8(data);

            if (!filePath.isEmpty()) {
                qDebug() << "Received external request to play:" << filePath;

                w.show();
                w.raise();
                w.activateWindow();
                w.start_playback(filePath);
            }
            });
        });

    if (!server.listen(serverName)) {
        qWarning() << "Unable to start local server:" << server.errorString();
    }


    try {
        py::module_ sys = py::module_::import("sys");
        sys.attr("path").attr("append")(".");

        py::module_ my_module = py::module_::import("PAW_python");
        qDebug() << "Python API loaded successfully.";
    }
    catch (const std::exception& e) {
        qCritical() << "Failed to load Python API:" << e.what();
    }

    QIcon appIcon;
    appIcon.addFile(":/assets/icon_64.png");
    appIcon.addFile(":/assets/icon_256.png");
    a.setWindowIcon(appIcon);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCritical() << "PortAudio initialization error:" << Pa_GetErrorText(err);
    }

    w.show();

    py::gil_scoped_release release;

#ifdef Q_OS_LINUX
    LinuxKeys* keys = new LinuxKeys(&w);
    QObject::connect(keys, &LinuxKeys::playPauseRequested, &w, &Main_PAW_widget::PlayPauseButton);
    QObject::connect(keys, &LinuxKeys::nextRequested, &w, &Main_PAW_widget::PlayNextItem);
    QObject::connect(keys, &LinuxKeys::previousRequested, &w, &Main_PAW_widget::PlayPreviousItem);
    QObject::connect(keys, &LinuxKeys::stopRequested, &w, &Main_PAW_widget::StopPlayback);

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerService("org.mpris.MediaPlayer2.perfectaudioworks");
    bus.registerObject("/org/mpris/MediaPlayer2", &w);
#endif

    QStringList args = QCoreApplication::arguments();
    if (args.count() > 1) {
        w.start_playback(args[1]);
    }

    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&w]() {
        w.getAudioThread().stopPlayback();
        global_paw_widget = nullptr;
        });

    int result = a.exec();

    Pa_Terminate();

    return result;

}