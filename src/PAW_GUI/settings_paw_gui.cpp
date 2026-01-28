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

    auto applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);
    if (applyButton) {
        connect(applyButton, &QPushButton::clicked, this, &Settings_PAW_gui::applySettings);
    }

    if (m_audiothread) {
        QList<QPair<QString, int>> availableDevices = m_audiothread->GetAllAvailableOutputDevices();
        for (const auto& device : availableDevices) {
            ui->audioDeviceComboBox->addItem(device.first, device.second);
        }
    }

    ui->settingsMenu->setCurrentRow(0);
}

Settings_PAW_gui::~Settings_PAW_gui()
{
    delete ui;
}

void Settings_PAW_gui::applySettings() {
    if (m_audiothread) {
        int selectedPaDeviceIndex = ui->audioDeviceComboBox->currentData().toInt();
        m_audiothread->setAudioDevice(selectedPaDeviceIndex);

        qDebug() << "Applied Audio Device Index:" << selectedPaDeviceIndex;
    }
    else {
        qWarning() << "PortaudioThread is null, cannot set audio device.";
    }
}

