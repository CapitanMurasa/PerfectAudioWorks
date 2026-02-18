#pragma once
#pragma push_macro("slots")
#undef slots

#include <pybind11/pybind11.h>
#include <pybind11/functional.h> 

#pragma pop_macro("slots")

#include <QObject>
#include <QTimer>
#include <vector>
#include "../PAW_GUI/Enums.h"
#include "../AudioPharser/PortAudioHandler.h"

class Main_PAW_widget;

namespace py = pybind11;

struct PythonCallback {
    py::function func;
    QTimer* timer;
};

class PythonEventThread : public QObject {
    Q_OBJECT

public:
    PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget);
    ~PythonEventThread();

    void registerCallback(py::function callback, int interval_ms);

public slots:
    void loadPluginAsync(const QString& filePath);
    void InitializePlugin(const QString& name);
    void clearAllCallbacks();

signals:
    void pluginLoadFinished(bool success, QString filePath, QString fileName, QString pluginName);
    void RequestMessageBox(Messagetype type, QString message);

private:
    bool openPluginInternal(const QString& filePath);

    QString pluginname;
    std::vector<PythonCallback*> activeCallbacks;
};