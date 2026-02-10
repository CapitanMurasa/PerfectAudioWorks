#ifndef SETTINGS_PAW_GUI_H
#define SETTINGS_PAW_GUI_H

#include "../miscellaneous/PythonEvent.h"
#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/python_bindings.h" 
#include "../miscellaneous/json.h"

#include <QDialog>  
#include <QDebug>
#include <QComboBox> 
#include <QThread>




class Main_PAW_widget;

QT_BEGIN_NAMESPACE
namespace Ui {
    class Settings_PAW_gui;
}
QT_END_NAMESPACE

class Settings_PAW_gui : public QDialog
{
    Q_OBJECT

public:
    explicit Settings_PAW_gui(PortaudioThread* audioThread, Main_PAW_widget* parent = nullptr);
    ~Settings_PAW_gui() override;

    json settings;

    json pluginsList;

private slots:
    void applySettings();
    void addplugins();
    void onPluginLoaded(bool success, QString filePath, QString fileName, QString Pluginname);

signals:
    void requestLoadPlugin(QString path);



private:
    Ui::Settings_PAW_gui* ui;

    QThread* m_pythonThread;
    PythonEventThread* m_pyWorker;

    Main_PAW_widget* mainwidget;
    PortaudioThread* m_audiothread;

    JsonLoader loader;

    void SetupJson();

    void addPluginsfromJson();

    bool ProcessPlugin(const QString& file);
};

#endif 