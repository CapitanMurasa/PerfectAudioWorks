#include "settings_paw_gui.h"
#include "ui_settings_paw_gui.h"
#include <QPushButton>

Settings_PAW_gui::Settings_PAW_gui(PortaudioThread* audioThread, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Settings_PAW_gui)
    , m_audiothread(audioThread)
{
    ui->setupUi(this);

    connect(ui->settingsMenu, &QListWidget::currentRowChanged,
        ui->settingsStack, &QStackedWidget::setCurrentIndex);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Settings_PAW_gui::applySettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Settings_PAW_gui::close);

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


    if (settings.contains("audio_device_index")) {
        int savedIdx = settings["audio_device_index"].get<int>();
        int uiIdx = ui->audioDeviceComboBox->findData(savedIdx);
        if (uiIdx != -1) {
            ui->audioDeviceComboBox->setCurrentIndex(uiIdx);
        }
    }

}

void Settings_PAW_gui::applySettings() {
    if (m_audiothread) {
        int selectedPaDeviceIndex = ui->audioDeviceComboBox->currentData().toInt();
        m_audiothread->setAudioDevice(selectedPaDeviceIndex);

        settings["audio_device_index"] = selectedPaDeviceIndex;
    }


    if (ui->SavePlaylistsCheck->isChecked()) {
        settings["save_playlists"] = true;
    }
    else {
        settings["save_playlists"] = false;
    }

    loader.save_config(settings, "settings.json");
}

