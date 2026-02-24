#include "aboutfile_paw_gui.h"
#include "ui_aboutfile_paw_gui.h" 
#include "../miscellaneous/file.h"

Aboutfile_PAW_gui::Aboutfile_PAW_gui(DatabaseManager* db, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::Aboutfile_PAW_gui),
    m_database(db)
{
    ui->setupUi(this);
}

Aboutfile_PAW_gui::~Aboutfile_PAW_gui()
{
    delete ui;
}

void Aboutfile_PAW_gui::setdata(QTableWidgetItem* item) {
    if (item) {
        QString path = item->data(Qt::UserRole).toString();
        if (path.isEmpty()) path = item->text();

        setdata(path);
    }
}

void Aboutfile_PAW_gui::setdata(const QString& filename) {
    FileInfo filemetadata = { 0 };
    int metadata_result = -1;

#ifdef _WIN32
    std::wstring w_filePath = filename.toStdWString();
    metadata_result = get_metadata_w(w_filePath.c_str(), &filemetadata);
#else
    QByteArray utf8_filePath = filename.toUtf8();
    metadata_result = get_metadata(utf8_filePath.constData(), &filemetadata);
#endif

    TrackData trackInfo = m_database->LoadRow(filename);

    if (trackInfo.title.isEmpty() && metadata_result == 0) {
        m_database->FillRow(filemetadata, filename);
        trackInfo = m_database->LoadRow(filename);
    }

    if (!trackInfo.title.isEmpty()) {
        ui->val_name->setText(trackInfo.title);
        ui->val_artist->setText(trackInfo.artist.isEmpty() ? "Unknown Artist" : trackInfo.artist);
        ui->val_album->setText(trackInfo.album.isEmpty() ? "Unknown Album" : trackInfo.album);
        ui->val_genre->setText(trackInfo.genre.isEmpty() ? "-" : trackInfo.genre);
        ui->val_type->setText(trackInfo.format.isEmpty() ? "AUDIO" : trackInfo.format.toUpper());

        if (!trackInfo.coverImage.isEmpty()) {
            m_originalAlbumArt.loadFromData(trackInfo.coverImage, "JPG");
        }
        else {
            m_originalAlbumArt = QPixmap();
        }
    }
    else {
        ui->val_name->setText(filename.section('/', -1));
        ui->val_artist->setText("-");
        ui->val_album->setText("-");
        ui->val_genre->setText("-");
        ui->val_type->setText("-");
        m_originalAlbumArt = QPixmap();
    }

    ui->val_location->setText(filename);

    if (metadata_result == 0) {
        if (filemetadata.channels == 1) {
            ui->val_channels->setText("Mono (1)");
        }
        else if (filemetadata.channels == 2) {
            ui->val_channels->setText("Stereo (2)");
        }
        else {
            ui->val_channels->setText(QString::number(filemetadata.channels) + " channels");
        }

        ui->val_samplerate->setText(QString::number(filemetadata.sampleRate) + " Hz");
        ui->val_bitrate->setText(QString::number(filemetadata.bitrate) + " kbps");
    }
    else {
        ui->val_channels->setText("-");
        ui->val_samplerate->setText("-");
        ui->val_bitrate->setText("-");
    }

    FileInfo_cleanup(&filemetadata);
    updateAlbumArt();
}

void Aboutfile_PAW_gui::updateAlbumArt()
{
    if (m_originalAlbumArt.isNull()) {
        ui->val_art->setText("No Image");
        ui->val_art->setStyleSheet("background-color: #e0e0e0; border: 1px solid #c0c0c0; color: #555;");
        return;
    }

    ui->val_art->setText("");
    ui->val_art->setStyleSheet("border: 1px solid #c0c0c0;");

    QPixmap scaledArt = m_originalAlbumArt.scaled(ui->val_art->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    ui->val_art->setPixmap(scaledArt);
}

void Aboutfile_PAW_gui::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    updateAlbumArt();
}