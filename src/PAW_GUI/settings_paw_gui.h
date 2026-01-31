#ifndef Settings_PAW_gui_H
#define Settings_PAW_gui_H

#include <QMainWindow>
#include <QDebug>
#include <QComboBox> 

#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/json.h"

QT_BEGIN_NAMESPACE
namespace Ui {
    class Settings_PAW_gui;
}
QT_END_NAMESPACE

class Settings_PAW_gui : public QMainWindow
{
    Q_OBJECT

public:
    explicit Settings_PAW_gui(PortaudioThread* audioThread, QWidget* parent = nullptr);
    ~Settings_PAW_gui() override;
    json settings;

private slots:
    void applySettings();


private:
    Ui::Settings_PAW_gui* ui;
    PortaudioThread* m_audiothread;
    JsonLoader loader;

    void SetupJson();


};

#endif