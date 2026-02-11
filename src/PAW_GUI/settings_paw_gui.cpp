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
        qWarning() << "No settings found, starting with defaults.";
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

    if (ui->AddPluginsButton) {
        connect(ui->AddPluginsButton, &QPushButton::clicked, this, &Settings_PAW_gui::addplugins);
    }

    ui->settingsMenu->setCurrentRow(0);
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

void Settings_PAW_gui::SetupJson() {
    if (settings.contains("audio_device_index")) {
        int savedIdx = settings["audio_device_index"].get<int>();
        int uiIdx = ui->audioDeviceComboBox->findData(savedIdx);
        if (uiIdx != -1) ui->audioDeviceComboBox->setCurrentIndex(uiIdx);
    }

    if (settings.contains("save_playlists") && settings["save_playlists"].is_boolean()) {
        ui->SavePlaylistsCheck->setChecked(settings["save_playlists"].get<bool>());
    }

    bool autoSkip = true;
    if (settings.contains("auto_skip_tracks")) autoSkip = settings["auto_skip_tracks"].get<bool>();
    ui->AutoSkipTracks->setChecked(autoSkip);
    if (mainwidget) mainwidget->CanAutoSwitch = autoSkip;

    bool useExtArt = true;
    if (settings.contains("use_external_album_art")) useExtArt = settings["use_external_album_art"].get<bool>();
    ui->UseExternalAlbumArt->setChecked(useExtArt);
    setCanUseExternalAlbumart(useExtArt);

    bool UsePlugins = false;
    if (settings.contains("use_plugins")) UsePlugins = settings["use_plugins"].get<bool>();
    ui->enablePLuginsSupportCheckBox->setChecked(UsePlugins);
    usePlugins = UsePlugins;
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
    qDebug() << "Settings applied and saved.";
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

void Settings_PAW_gui::addPluginsfromJson() {
    ui->PluginsList->clear();

    if (!usePlugins) {
        return;
    }

    if (pluginsList.is_array()) {
        for (const auto& item : pluginsList) {
            QString filePath = QString::fromStdString(item.get<std::string>());
            emit requestLoadPlugin(filePath);
        }
    }
}

void Settings_PAW_gui::onPluginLoaded(bool success, QString filePath, QString fileName, QString Pluginname) {
    if (!success) return;

    for (int i = 0; i < ui->PluginsList->count(); ++i) {
        if (ui->PluginsList->item(i)->data(Qt::UserRole).toString() == filePath)
            return;
    }

    QString displayText = Pluginname.isEmpty() ? fileName : Pluginname;

    QListWidgetItem* item = new QListWidgetItem(displayText);

    item->setData(Qt::UserRole, filePath); 
    item->setToolTip(filePath);

    ui->PluginsList->addItem(item);

    std::string stdPath = filePath.toStdString();
    bool exists = false;
    for (const auto& item : pluginsList) {
        if (item.get<std::string>() == stdPath) { exists = true; break; }
    }

    if (!exists) {
        pluginsList.push_back(stdPath);
        loader.save_config(pluginsList, "plugins.json");
    }
}