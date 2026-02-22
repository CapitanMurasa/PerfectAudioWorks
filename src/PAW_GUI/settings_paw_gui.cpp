#include "settings_paw_gui.h"
#include "ui_settings_paw_gui.h"
#include "main_paw_widget.h"
#include "../AudioPharser/PortAudioHandler.h"

#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QCoreApplication>
#include <QDebug>
#include <QMenu>

namespace py = pybind11;

Settings_PAW_gui::Settings_PAW_gui(PortaudioThread* audioThread, Main_PAW_widget* parent)
    : QDialog(parent)
    , ui(new Ui::Settings_PAW_gui)
    , m_audiothread(audioThread)
    , mainwidget(parent)
{
    ui->setupUi(this);

    m_pythonThread = new QThread(this);
    m_pyWorker = new PythonEventThread(audioThread, parent);

    global_pyevent = m_pyWorker;

    m_pyWorker->moveToThread(m_pythonThread);



    connect(this, &Settings_PAW_gui::requestLoadPlugin,
        m_pyWorker, &PythonEventThread::loadPluginAsync);

    connect(m_pyWorker, &PythonEventThread::pluginLoadFinished,
        this, &Settings_PAW_gui::onPluginLoaded);

    connect(m_pyWorker, &PythonEventThread::RequestMessageBox,
        this, &Settings_PAW_gui::ShowMessageBox);

    m_pythonThread->start();

    connect(ui->settingsMenu, &QListWidget::currentRowChanged,
        ui->settingsStack, &QStackedWidget::setCurrentIndex);

    if (m_audiothread) {
        ui->audioDeviceComboBox->clear();
        QList<QPair<QString, int>> availableDevices = m_audiothread->GetAllAvailableOutputDevices();
        for (const auto& device : availableDevices) {
            ui->audioDeviceComboBox->addItem(device.first, device.second);
        }
    }

    if (loader.load_jsonfile(settings, "settings.json")) {
        SetupJson();
    }
    else {
        settings["audio_device_index"] = -1;
        settings["save_playlists"] = true;
        settings["auto_skip_tracks"] = true;
        settings["use_external_album_art"] = true;
        settings["use_plugins"] = false;

        loader.save_config(settings, "settings.json");

        SetupJson();
    }

    if (!loader.load_jsonfile(pluginsList, "plugins.json")) {
        pluginsList = nlohmann::json::array();
    }
    else {
        addPluginsfromJson();
    }

    QPushButton* applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyButton) {
        connect(applyButton, &QPushButton::clicked, this, &Settings_PAW_gui::applySettings);
    }

    connect(this, &QDialog::accepted, this, &Settings_PAW_gui::applySettings);

    connect(ui->AddPluginsButton, &QPushButton::clicked, this, &Settings_PAW_gui::addplugins);
    connect(ui->reloadPluginsButton, &QPushButton::clicked, this, &Settings_PAW_gui::reloadplugins);
    connect(ui->PluginsList, &QListWidget::customContextMenuRequested, this, &Settings_PAW_gui::showPlaylistContextMenu);

    SetupQtActions();

    ui->settingsMenu->setCurrentRow(0);
    ui->PluginsList->setContextMenuPolicy(Qt::CustomContextMenu);
}

Settings_PAW_gui::~Settings_PAW_gui()
{
    global_pyevent = nullptr;

    if (m_pythonThread->isRunning()) {
        m_pythonThread->quit();
        m_pythonThread->wait();
    }


    delete m_pyWorker;
    m_pyWorker = nullptr;

    delete m_pythonThread;
    m_pythonThread = nullptr;

    delete ui;
}

void Settings_PAW_gui::SetupQtActions() {
    m_disableAction = new QAction("Disable Plugin", this);
    m_disableAction->setShortcut(QKeySequence::Delete);
    m_disableAction->setShortcutContext(Qt::WidgetShortcut);
    connect(m_disableAction, &QAction::triggered, this, &Settings_PAW_gui::disablePlugin);
    ui->PluginsList->addAction(m_disableAction);

    m_deleteAction = new QAction("Delete Plugin", this);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setShortcutContext(Qt::WidgetShortcut);
    connect(m_deleteAction, &QAction::triggered, this, &Settings_PAW_gui::deletePlugin);
    ui->PluginsList->addAction(m_deleteAction);
}

void Settings_PAW_gui::SetupJson() {
    int savedIdx = settings.value("audio_device_index", -1);
    int uiIdx = ui->audioDeviceComboBox->findData(savedIdx);
    if (uiIdx != -1){
        ui->audioDeviceComboBox->setCurrentIndex(uiIdx);
        int deviceId = ui->audioDeviceComboBox->itemData(uiIdx).toInt();
        m_audiothread->changeAudioDevice(deviceId);
    }

    bool savePl = settings.value("save_playlists", true);
    ui->SavePlaylistsCheck->setChecked(savePl);

    bool autoSkip = settings.value("auto_skip_tracks", true);
    ui->AutoSkipTracks->setChecked(autoSkip);
    if (mainwidget) mainwidget->CanAutoSwitch = autoSkip;

    bool useExtArt = settings.value("use_external_album_art", true);
    ui->UseExternalAlbumArt->setChecked(useExtArt);
    setCanUseExternalAlbumart(useExtArt);

    bool usePlugs = settings.value("use_plugins", false);
    ui->enablePLuginsSupportCheckBox->setChecked(usePlugs);
    this->usePlugins = usePlugs;
}

void Settings_PAW_gui::applySettings() {
    if (m_audiothread) {
        int selectedPaDeviceIndex = ui->audioDeviceComboBox->currentData().toInt();
        m_audiothread->changeAudioDevice(selectedPaDeviceIndex);
        settings["audio_device_index"] = selectedPaDeviceIndex;
    }

    settings["save_playlists"] = ui->SavePlaylistsCheck->isChecked();

    bool autoSkip = ui->AutoSkipTracks->isChecked();
    settings["auto_skip_tracks"] = autoSkip;
    if (mainwidget) mainwidget->CanAutoSwitch = autoSkip;

    bool useExtArt = ui->UseExternalAlbumArt->isChecked();
    settings["use_external_album_art"] = useExtArt;
    setCanUseExternalAlbumart(useExtArt);

    bool UsePlugins = ui->enablePLuginsSupportCheckBox->isChecked();
    settings["use_plugins"] = UsePlugins;
    usePlugins = UsePlugins;

    loader.save_config(settings, "settings.json");
    ShowMessageBox(PAW_INFO, "Settings applied", "Settings applied and saved.");
}

void Settings_PAW_gui::addplugins() {
    if (!usePlugins) {
        return;
    }
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Python Plugins", "", "Python Files (*.py)");

    for (const QString& file : files) {
        if (file.isEmpty()) continue;
        emit requestLoadPlugin(file);

    }
}

void Settings_PAW_gui::reloadplugins() {
    if (!usePlugins) return;

    QMetaObject::invokeMethod(m_pyWorker, "clearAllCallbacks", Qt::QueuedConnection);

    QStringList pathsToReload;
    for (int i = 0; i < ui->PluginsList->count(); ++i) {
        pathsToReload << ui->PluginsList->item(i)->data(Qt::UserRole).toString();
    }

    ui->PluginsList->clear();

    for (const QString& file : pathsToReload) {
        emit requestLoadPlugin(file);
    }
}

void Settings_PAW_gui::deletePlugin() {
    if (!usePlugins) {
        return;
    }

    int currentRow = ui->PluginsList->currentRow();

    if (currentRow >= 0)
    {

        QListWidgetItem* itemToDelete = ui->PluginsList->item(currentRow);


        if (pluginsList.is_array() && currentRow < pluginsList.size()) {
            loader.RemoveItemByIndex(pluginsList, currentRow);
            loader.save_config(pluginsList, "plugins.json");
        }

        delete ui->PluginsList->takeItem(currentRow);
    }

}

void Settings_PAW_gui::disablePlugin() {
    if (!usePlugins) return;

    int currentRow = ui->PluginsList->currentRow();
    if (currentRow < 0 || !pluginsList.is_array() || currentRow >= (int)pluginsList.size()) {
        return;
    }

    QListWidgetItem* item = ui->PluginsList->item(currentRow);
    QString filePath = item->data(Qt::UserRole).toString();

    bool nowEnabled = !pluginsList[currentRow].value("enabled", true);
    pluginsList[currentRow]["enabled"] = nowEnabled;
    loader.save_config(pluginsList, "plugins.json");

    if (nowEnabled) {
        QString cleanName = item->text().remove(" (Disabled)");
        item->setText(cleanName);
        item->setForeground(Qt::black);
        emit requestLoadPlugin(filePath);
    }
    else {
        if (!item->text().endsWith(" (Disabled)")) {
            item->setText(item->text() + " (Disabled)");
        }
        item->setForeground(Qt::gray);
    }
}

void Settings_PAW_gui::showPlaylistContextMenu(const QPoint& pos) {
    QListWidgetItem* clickedItem = ui->PluginsList->itemAt(pos);
    if (!clickedItem) return;

    bool isDisabled = clickedItem->text().endsWith(" (Disabled)");
    m_disableAction->setText(isDisabled ? "Enable Plugin" : "Disable Plugin");

    QMenu contextMenu(this);
    m_deleteAction->setEnabled(true);
    m_disableAction->setEnabled(true);

    contextMenu.addAction(m_disableAction);
    contextMenu.addSeparator();
    contextMenu.addAction(m_deleteAction);

    contextMenu.exec(ui->PluginsList->mapToGlobal(pos));
}

void Settings_PAW_gui::addPluginsfromJson() {
    ui->PluginsList->clear();

    if (!usePlugins) return;

    if (pluginsList.is_array()) {
        for (const auto& item : pluginsList) {
            std::string pathStr;
            bool isEnabled = true; 

            if (item.is_object()) {
                if (item.contains("path")) {
                    pathStr = item["path"].get<std::string>();
                }
                if (item.contains("enabled") && item["enabled"].is_boolean()) {
                    isEnabled = item["enabled"].get<bool>();
                }
            }
            else if (item.is_string()) {
                pathStr = item.get<std::string>();
            }

            if (!pathStr.empty()) {
                QString filePath = QString::fromStdString(pathStr);


                if (isEnabled) {
                    emit requestLoadPlugin(filePath);
                }
                else {
                    QListWidgetItem* listItem = new QListWidgetItem(QFileInfo(filePath).fileName() + " (Disabled)");
                    listItem->setData(Qt::UserRole, filePath);
                    listItem->setForeground(Qt::gray);
                    listItem->setToolTip(filePath);
                    ui->PluginsList->addItem(listItem);
                }
            }
        }
    }
}

void Settings_PAW_gui::ShowMessageBox(Messagetype type, QString title, QString message) {
    switch (type) {
    case PAW_WARNING:
        QMessageBox::warning(this, title, message);
        break;
    case PAW_ERROR:
        QMessageBox::critical(this, title, message);
        break;
    case PAW_INFO:
        QMessageBox::information(this, title, message);
        break;
    }
}

void Settings_PAW_gui::onPluginLoaded(bool success, QString filePath, QString fileName, QString Pluginname) {
    if (!success) return;

    for (int i = 0; i < ui->PluginsList->count(); ++i) {
        if (ui->PluginsList->item(i)->data(Qt::UserRole).toString() == filePath)
            return;
    }

    std::string stdPath = filePath.toStdString();
    std::string stdName = Pluginname.isEmpty() ? fileName.toStdString() : Pluginname.toStdString();
    bool isEnabled = true; 
    bool exists = false;

    if (pluginsList.is_array()) {
        for (auto& pluginObj : pluginsList) {
            if (pluginObj.is_object() && pluginObj.contains("path") && pluginObj["path"] == stdPath) {
                pluginObj["name"] = stdName;
                isEnabled = pluginObj.value("enabled", true);
                exists = true;
                break;
            }
            else if (pluginObj.is_string() && pluginObj == stdPath) {
                pluginObj = {
                    {"path", stdPath},
                    {"name", stdName},
                    {"version", "1.2"},
                    {"enabled", true}
                };
                isEnabled = true;
                exists = true;
                break;
            }
        }
    }

    if (!exists) {
        json newPlugin = {
            {"path", stdPath},
            {"name", stdName},
            {"version", "1.2"},
            {"enabled", true}
        };
        pluginsList.push_back(newPlugin);
        loader.save_config(pluginsList, "plugins.json");
    }

    QString displayText = QString::fromStdString(stdName);
    QListWidgetItem* listItem = new QListWidgetItem(displayText);
    listItem->setData(Qt::UserRole, filePath);
    listItem->setToolTip(filePath);

    if (isEnabled) {
        listItem->setForeground(Qt::black);
    }
    else {
        listItem->setForeground(Qt::gray);
        listItem->setText(displayText + " (Disabled)");
    }

    ui->PluginsList->addItem(listItem);
}