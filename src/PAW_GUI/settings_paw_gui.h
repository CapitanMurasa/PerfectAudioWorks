#ifndef SETTINGS_PAW_GUI_H
#define SETTINGS_PAW_GUI_H

#include <QDialog>  
#include <QDebug>
#include <QComboBox> 

#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/json.h"
#include "../miscellaneous/PythonEvent.h"


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


private:
    Ui::Settings_PAW_gui* ui;
    PortaudioThread* m_audiothread;

    PythonEventThread* pythread;

    Main_PAW_widget* mainwidget;

    JsonLoader loader;

    void SetupJson();

    void addPluginsfromJson();

    bool ProcessPlugin(const QString& file);
};

#endif 