#include <pybind11/embed.h>

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

    loader.save_config(settings, "settings.json");
    qDebug() << "Settings applied and saved.";
}


void Settings_PAW_gui::addplugins() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Select Python Plugins", "", "Python Files (*.py)");

    bool listChanged = false;

    for (const QString& file : files) {
        if (file.isEmpty()) continue;

        bool success = ProcessPlugin(file);

        if (success) {
            QFileInfo fi(file);
            std::string stdName = fi.fileName().toStdString();

            bool exists = false;
            for (const auto& item : pluginsList) {
                if (item.get<std::string>() == stdName) { exists = true; break; }
            }

            if (!exists) {
                pluginsList.push_back(stdName);
                listChanged = true;
            }
        }
    }

    if (listChanged) {
        loader.save_config(pluginsList, "plugins.json");
    }
}

void Settings_PAW_gui::addPluginsfromJson() {
    ui->PluginsList->clear();

    if (pluginsList.is_array()) {
        for (const auto& item : pluginsList) {
            QString fileName = QString::fromStdString(item.get<std::string>());

            QString localPath = QCoreApplication::applicationDirPath() + "/plugins/" + fileName;

            ProcessPlugin(localPath);
        }
    }
}

bool Settings_PAW_gui::ProcessPlugin(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) return false;

    QString fileName = fileInfo.fileName();
    QString moduleName = fileInfo.completeBaseName(); 

    QString appPath = QCoreApplication::applicationDirPath();
    QDir pluginsDir(appPath + "/plugins");
    if (!pluginsDir.exists()) pluginsDir.mkpath(".");

    QString destPath = pluginsDir.filePath(fileName);
    if (QFileInfo(filePath).absoluteFilePath() != QFileInfo(destPath).absoluteFilePath()) {
        if (QFile::exists(destPath)) QFile::remove(destPath);

        if (!QFile::copy(filePath, destPath)) {
            QMessageBox::warning(this, "Error", "Could not copy plugin to plugins folder.");
            return false;
        }
    }

    try {
        py::module_ sys = py::module_::import("sys");
        std::string stdPluginsPath = pluginsDir.absolutePath().toStdString();

        bool inPath = false;
        for (auto p : sys.attr("path")) {
            if (p.cast<std::string>() == stdPluginsPath) { inPath = true; break; }
        }
        if (!inPath) sys.attr("path").attr("append")(stdPluginsPath);

        py::module_ mod = py::module_::import(moduleName.toStdString().c_str());

        py::module_ importlib = py::module_::import("importlib");
        importlib.attr("reload")(mod);

        qDebug() << "Loaded Plugin:" << moduleName;


        bool inUi = false;
        for (int i = 0; i < ui->PluginsList->count(); ++i) {
            if (ui->PluginsList->item(i)->text() == fileName) { inUi = true; break; }
        }

        if (!inUi) {
            QListWidgetItem* item = new QListWidgetItem(fileName);
            item->setData(Qt::UserRole, destPath); 
            ui->PluginsList->addItem(item);
        }

        return true;

    }
    catch (const std::exception& e) {
        qCritical() << "Python Plugin Error:" << e.what();
        QMessageBox::critical(this, "Plugin Error", QString("Failed to load %1:\n%2").arg(fileName).arg(e.what()));
        return false;
    }
}