#pragma once
#pragma push_macro("slots")
#undef slots
#include <pybind11/pybind11.h>
#include <pybind11/functional.h> 
#pragma pop_macro("slots")

#include <QObject>
#include <QTimer>
#include <QString>
#include <vector>
#include "../PAW_GUI/Enums.h"

class Main_PAW_widget;
class PortaudioThread;
class PythonEventThread; 
extern PythonEventThread* global_pyevent;

namespace py = pybind11;

struct PythonCallback {
    py::function func;
    QTimer* timer;
    QString ownerPath; 
};

class PythonEventThread : public QObject {
    Q_OBJECT

public:
    PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget);
    ~PythonEventThread();

    void registerCallback(py::function callback, int interval_ms);

public slots:
    void loadPluginAsync(const QString& filePath);
    void unloadPlugin(const QString& filePath); 
    void InitializePlugin(const QString& name);
    void sendMessagebox(Messagetype type, QString message);
    void clearAllCallbacks();

signals:
    void pluginLoadFinished(bool success, QString filePath, QString fileName, QString pluginName);
    void RequestMessageBox(Messagetype type, QString message);

private:
    bool openPluginInternal(const QString& filePath);

    QString m_pluginName;        
    QString m_currentLoadingPath; 
    std::vector<PythonCallback*> activeCallbacks;
};