#pragma once
#pragma push_macro("slots")
#undef slots

#include <pybind11/embed.h>

#pragma pop_macro("slots")
#include <QObject>
//#include "../PAW_GUI/main_paw_widget.h"
#include "../AudioPharser/PortAudioHandler.h"

class PythonEventThread : public QObject {
    Q_OBJECT

public:
    PythonEventThread(PortaudioThread* audioThread);
    ~PythonEventThread();
    bool openPlugin(const QString& filePath);
};