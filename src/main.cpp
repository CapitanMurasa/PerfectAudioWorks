
#include "PAW_GUI/main_paw_widget.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMessageBox>

#ifdef Q_OS_LINUX
#include "PAW_GUI/GlobalLinuxKeys.h"
#endif

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setlocale(LC_ALL, ".UTF-8");

    QApplication a(argc, argv);

    QIcon appIcon;
    appIcon.addFile(":/assets/icon_64.png");
    appIcon.addFile(":/assets/icon_256.png");

    a.setWindowIcon(appIcon);

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCritical() << "PortAudio initialization error:" << Pa_GetErrorText(err);
    }

    Main_PAW_widget w;
    w.show();

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
        });

    int result = a.exec();

    Pa_Terminate();

    return result;
}
