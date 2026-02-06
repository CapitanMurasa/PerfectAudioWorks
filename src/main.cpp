
#include "PAW_GUI/main_paw_widget.h"
#include <QApplication>
#include <QIcon>
#include <QDebug>

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