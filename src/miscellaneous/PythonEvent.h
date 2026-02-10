#pragma once
#pragma push_macro("slots")
#undef slots

#include <pybind11/embed.h>

#pragma pop_macro("slots")
#include <QObject>
#include "../AudioPharser/PortAudioHandler.h"

class Main_PAW_widget;

class PythonEventThread : public QObject {
    Q_OBJECT

public:
    PythonEventThread(PortaudioThread* audioThread, Main_PAW_widget* mainWidget);
    ~PythonEventThread();

public slots:
    void loadPluginAsync(const QString& filePath);

signals:
    void pluginLoadFinished(bool success, QString filePath, QString fileName);

private:
    bool openPluginInternal(const QString& filePath);
};