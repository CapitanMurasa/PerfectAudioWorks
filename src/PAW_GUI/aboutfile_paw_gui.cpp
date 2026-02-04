#include "aboutfile_paw_gui.h"
#include "ui_aboutfile_paw_gui.h" 
#include "../miscellaneous/file.h"

Aboutfile_PAW_gui::Aboutfile_PAW_gui(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::Aboutfile_PAW_gui)
{
    ui->setupUi(this);
}

Aboutfile_PAW_gui::~Aboutfile_PAW_gui()
{
    delete ui;
}

void Aboutfile_PAW_gui::setdata(QListWidgetItem* item) {
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

    if (metadata_result == 0) {
        QPixmap coverArt;
        bool artFound = false;


        QString title = (filemetadata.title[0] != '\0')
            ? QString::fromUtf8(filemetadata.title)
            : filename.section('/', -1);

        QString artist = (filemetadata.artist[0] != '\0')
            ? QString::fromUtf8(filemetadata.artist)
            : "Unknown Artist";

        QString album = (filemetadata.album[0] != '\0')
            ? QString::fromUtf8(filemetadata.album)
            : "Unknown Album";

        QString genre = (filemetadata.genre[0] != '\0')
            ? QString::fromUtf8(filemetadata.genre)
            : "-";

        QString format = (filemetadata.format[0] != '\0')
            ? QString::fromUtf8(filemetadata.format).toUpper()
            : "AUDIO";


        if (filemetadata.cover_image && filemetadata.cover_size > 0) {

            if (coverArt.loadFromData(filemetadata.cover_image, static_cast<uint>(filemetadata.cover_size))) {
                artFound = true;
            }
        }


        ui->val_name->setText(title);
        ui->val_album->setText(album);
        ui->val_artist->setText(artist);
        ui->val_genre->setText(genre);
        ui->val_location->setText(filename);
        ui->val_type->setText(format);


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


        FileInfo_cleanup(&filemetadata);


        m_originalAlbumArt = artFound ? coverArt : QPixmap();
    }
    else {
        ui->val_name->setText(filename.section('/', -1));
        ui->val_album->setText("-");
        ui->val_artist->setText("-");
        ui->val_genre->setText("-");
        ui->val_location->setText(filename);
        ui->val_type->setText("-");
        ui->val_channels->setText("-");
        ui->val_samplerate->setText("-");
        ui->val_bitrate->setText("-");

        m_originalAlbumArt = QPixmap();
    }

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