#ifndef SETTINGS_PAW_GUI_H
#define SETTINGS_PAW_GUI_H

#include "../miscellaneous/PythonEvent.h"
#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/json.h"
#include "Enums.h"

#include <QDialog>  
#include <QThread>
#include <QAction>

Q_DECLARE_METATYPE(Messagetype)

QT_BEGIN_NAMESPACE
namespace Ui { class Settings_PAW_gui; }
QT_END_NAMESPACE

class Main_PAW_widget;

class Settings_PAW_gui : public QDialog
{
    Q_OBJECT

public:
    explicit Settings_PAW_gui(PortaudioThread* audioThread, Main_PAW_widget* parent = nullptr);
    ~Settings_PAW_gui() override;

    nlohmann::json settings;
    nlohmann::json pluginsList;

private slots:
    void applySettings();
    void addplugins();
    void reloadplugins();
    void deletePlugin();
    void disablePlugin();
    void showPlaylistContextMenu(const QPoint& pos);

    void onPluginLoaded(bool success, QString filePath, QString fileName, QString pluginName);
    void ShowMessageBox(Messagetype type, QString title, QString message);

signals:
    void requestLoadPlugin(QString path);

private:
    Ui::Settings_PAW_gui* ui;

    QThread* m_pythonThread;
    PythonEventThread* m_pyWorker;

    Main_PAW_widget* mainwidget;
    PortaudioThread* m_audiothread;
    JsonLoader loader;

    QAction* m_deleteAction;
    QAction* m_disableAction;
    bool usePlugins;

    void SetupQtActions();
    void SetupJson();
    void addPluginsfromJson();
    void LoadDefaults();
};

#endif