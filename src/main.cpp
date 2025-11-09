#include <iostream>

#include "PAW_GUI/main_paw_widget.h"

#include <QApplication>

int main(int argc, char* argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);
    setlocale(LC_ALL, ".UTF-8");
    QApplication a(argc, argv);
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qCritical() << "PortAudio initialization error:" << Pa_GetErrorText(err);
        
    }
    Main_PAW_widget w;

    w.show();

    if (argc > 1){
        w.start_playback(QString(argv[1]));
    }


    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&w]() {
        w.getAudioThread().stopPlayback(); 
    });

    return a.exec();
} 