#include "settings_paw_gui.h"
#include "ui_settings_paw_gui.h"
#include "../AudioPharser/PortAudioHandler.h"
#include <QPushButton>

Settings_PAW_gui::Settings_PAW_gui(PortaudioThread* audioThread, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Settings_PAW_gui)
    , m_audiothread(audioThread)
{
    ui->setupUi(this);

    connect(ui->settingsMenu, &QListWidget::currentRowChanged,
        ui->settingsStack, &QStackedWidget::setCurrentIndex);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Settings_PAW_gui::applySettingsandExit);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Settings_PAW_gui::close);

    mainwidget = qobject_cast<Main_PAW_widget*>(parent);

    if (m_audiothread) {
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

    auto applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyButton) {
        connect(applyButton, &QPushButton::clicked, this, &Settings_PAW_gui::applySettings);
    }

    ui->settingsMenu->setCurrentRow(0);
}

Settings_PAW_gui::~Settings_PAW_gui()
{
    delete ui;
}

void Settings_PAW_gui::SetupJson() {
    if (settings.contains("save_playlists") && settings["save_playlists"].is_boolean()) {
        ui->SavePlaylistsCheck->setChecked(settings["save_playlists"].get<bool>());
    }

    bool autoSkip = settings.value("auto_skip_tracks", true); 
    ui->AutoSkipTracks->setChecked(autoSkip);
    settings["auto_skip_tracks"] = autoSkip;
    if (mainwidget) mainwidget->CanAutoSwitch = autoSkip;

    bool useExtArt = settings.value("use_external_album_art", true);
    ui->UseExternalAlbumArt->setChecked(useExtArt);
    settings["use_external_album_art"] = useExtArt;
    setCanUseExternalAlbumart(useExtArt);

    if (settings.contains("audio_device_index")) {
        int savedIdx = settings["audio_device_index"].get<int>();
        int uiIdx = ui->audioDeviceComboBox->findData(savedIdx);

        if (uiIdx != -1) {
            ui->audioDeviceComboBox->setCurrentIndex(uiIdx);
            if (m_audiothread->isRunning()) {
                m_audiothread->changeAudioDevice(savedIdx);
            }
            else {
                m_audiothread->setAudioDevice(savedIdx); 
            }

        }
    }
}

void Settings_PAW_gui::applySettings() {
    if (m_audiothread) {
        int selectedPaDeviceIndex = ui->audioDeviceComboBox->currentData().toInt();
        m_audiothread->changeAudioDevice(selectedPaDeviceIndex);
        settings["audio_device_index"] = selectedPaDeviceIndex;
    }

    bool savePlaylists = ui->SavePlaylistsCheck->isChecked();
    settings["save_playlists"] = savePlaylists;

    bool autoSkip = ui->AutoSkipTracks->isChecked();
    settings["auto_skip_tracks"] = autoSkip;
    if (mainwidget) mainwidget->CanAutoSwitch = autoSkip;

    bool useExtArt = ui->UseExternalAlbumArt->isChecked();
    settings["use_external_album_art"] = useExtArt;
    setCanUseExternalAlbumart(useExtArt);

    loader.save_config(settings, "settings.json");
}

void Settings_PAW_gui::applySettingsandExit(){
    applySettings();

    this->close();
}
