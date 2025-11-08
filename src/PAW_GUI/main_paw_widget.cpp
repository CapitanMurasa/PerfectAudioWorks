#include "main_paw_widget.h"
#include "ui_main_paw_widget.h"
#include <cmath> 

Main_PAW_widget::Main_PAW_widget(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Main_PAW_widget)
{
    ui->setupUi(this);
    
    m_audiothread = new PortaudioThread(this);
    s = new Settings_PAW_gui(m_audiothread, this);
    
    connect(ui->TimelineSlider, &QSlider::valueChanged, this, &Main_PAW_widget::onSliderValueChanged);
    connect(ui->PlayPause, &QPushButton::clicked, this, &Main_PAW_widget::PlayPauseButton);
    connect(ui->actionSettings, &QAction::triggered, this, &Main_PAW_widget::openSettings);
    connect(ui->actionAbout, &QAction::triggered, this, &Main_PAW_widget::openAbout);
    connect(ui->actionadd_files_to_playlist, &QAction::triggered, this, &Main_PAW_widget::addFilesToPlaylist);
    connect(ui->Stop, &QPushButton::clicked, this, &Main_PAW_widget::StopPlayback);
    connect(ui->PreviousTrack, &QPushButton::clicked, this, &Main_PAW_widget::PlayPreviousItem);
    connect(ui->NextTrack, &QPushButton::clicked, this, &Main_PAW_widget::PlayNextItem);

    connect(m_audiothread, &PortaudioThread::playbackProgress, this, &Main_PAW_widget::handlePlaybackProgress);
    connect(m_audiothread, &PortaudioThread::totalFileInfo, this, &Main_PAW_widget::handleTotalFileInfo);
    connect(m_audiothread, &PortaudioThread::playbackFinished, this, &Main_PAW_widget::handlePlaybackFinished);
    connect(m_audiothread, &PortaudioThread::errorOccurred, this, &Main_PAW_widget::handleError);
    QObject::connect(ui->Playlist, &QListWidget::itemDoubleClicked, [&](QListWidgetItem *item){
        QString filename = item->data(Qt::UserRole).toString();
        qDebug() << "Selected file:" << filename;
        if (m_audiothread->isRunning()) {
            m_audiothread->stopPlayback();
            m_audiothread->wait();
        }
        start_playback(filename);
    });



   
    m_updateTimer = new QTimer(this);


    ui->TimelineSlider->setRange(0, 100); 
    ui->TimelineSlider->setValue(0);
    ui->CurrentFileDuration->setText("00:00");

}

Main_PAW_widget::~Main_PAW_widget()
{
   
    if (m_audiothread) {
        m_audiothread->stopPlayback();
        m_audiothread->wait();
        delete m_audiothread;
        m_audiothread = nullptr;
    }
    delete ui;
}

void Main_PAW_widget::start_playback(const QString &filename) {

    if (m_audiothread->isRunning()) {
        m_audiothread->stopPlayback();
    }

    m_currentFile = filename;
    m_audiothread->setFile(m_currentFile);

    QByteArray c_filePath = filename.toUtf8();
    const char* filenamechar = c_filePath.constData();


    if (get_metadata(filenamechar, &filemetadata) == 0) {
        QPixmap coverArt;
        bool artFound = false;

        QString title = filemetadata.title && strlen(filemetadata.title) > 0
                        ? QString::fromUtf8(filemetadata.title)
                        : m_currentFile.section('/', -1);
        QString artist = filemetadata.artist && strlen(filemetadata.artist) > 0
                         ? QString::fromUtf8(filemetadata.artist)
                         : "";

        if (filemetadata.cover_image && filemetadata.cover_size > 0) {
            if (coverArt.loadFromData(filemetadata.cover_image, filemetadata.cover_size)) {
                artFound = true;
            }
        }


        FileInfo_cleanup(&filemetadata);

        if (artFound) {
            m_originalAlbumArt = coverArt;
        }
        else {
            
            m_originalAlbumArt = QPixmap();
        }

        updateAlbumArt();

        ui->Filename->setText(title);
        ui->Artist->setText(artist);
    } else {

        ui->Filename->setText(m_currentFile.section('/', -1));
        ui->Artist->setText("");
    }

    m_audiothread->start(); 
    m_updateTimer->start(100); 
}


void Main_PAW_widget::updateAlbumArt()
{
    if (m_originalAlbumArt.isNull()) {

        ui->AlbumArt->setPixmap(QPixmap());
        ui->AlbumArt->setText("No Art");
        return;
    }


    QPixmap scaledArt = m_originalAlbumArt.scaled(ui->AlbumArt->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);


    ui->AlbumArt->setPixmap(scaledArt);
}

void Main_PAW_widget::resizeEvent(QResizeEvent* event)
{
    
    QWidget::resizeEvent(event);

    
    updateAlbumArt();
}

void Main_PAW_widget::handlePlaybackProgress(int currentFrame, int totalFrames, int sampleRate) {
    if (totalFrames > 0 && sampleRate > 0) {
        float framesInPercentage = (currentFrame * 1.0f) / totalFrames * 100.0f;
        float currentDuration = (currentFrame * 1.0f) / sampleRate;


        bool oldBlockState = ui->TimelineSlider->blockSignals(true);
        ui->TimelineSlider->setValue(static_cast<float>(framesInPercentage));
        ui->TimelineSlider->blockSignals(oldBlockState); 
        ui->CurrentFileDuration->setText(floatToMMSS(currentDuration));
    }
}


void Main_PAW_widget::handleTotalFileInfo(int totalFrames,int channels, int sampleRate, const char* codecname) {
    if (totalFrames > 0 && sampleRate > 0) {
        float totalDuration = (totalFrames * 1.0f) / sampleRate;
        ui->TotalFileDuration->setText(floatToMMSS(totalDuration));
        ui->SampleRateinfo->setText(QString::number(sampleRate));
        ui->CodecProcessorinfo->setText(QString::fromUtf8(codecname));
        if(channels == 1){
            ui->ChannelsInfo->setText("Mono");
        }
        else if (channels == 2){
            ui->ChannelsInfo->setText("Stereo");
        }
    }
}


void Main_PAW_widget::handlePlaybackFinished() {
    m_updateTimer->stop(); 
    ui->TimelineSlider->setValue(100); 
}




void Main_PAW_widget::on_actionopen_file_triggered() {
    QString filename = QFileDialog::getOpenFileName(this, "Open Audio File", "", "Audio Files (*.wav *.flac *.ogg *.opus *.mp3);;All Files (*)");
    if (!filename.isEmpty()) {
        start_playback(filename); 
    }
}


void Main_PAW_widget::onSliderValueChanged(float value) {

    m_audiothread->SetFrameFromTimeline(value);
}


void Main_PAW_widget::PlayPauseButton() {
    m_audiothread->setPlayPause();
    if (m_audiothread->isPaused()) {
        ui->PlayPause->setText("|>"); 
        m_updateTimer->stop(); 
    } else {
        ui->PlayPause->setText("||"); 
        m_updateTimer->start(100); 
    }
}

void Main_PAW_widget::StopPlayback(){
    if (m_audiothread->isRunning()) {
        m_audiothread->stopPlayback();
    }
}

void Main_PAW_widget::PlayNextItem() {
    QListWidgetItem* currentItem = ui->Playlist->currentItem();
    if (!currentItem) return;

    int currentRow = ui->Playlist->row(currentItem);
    int nextRow = currentRow + 1;
    if (nextRow >= ui->Playlist->count()) return;

    QListWidgetItem* nextItem = ui->Playlist->item(nextRow);
    QString filename = nextItem->data(Qt::UserRole).toString();  // get full path

    ui->Playlist->setCurrentItem(nextItem);
    start_playback(filename);
}

void Main_PAW_widget::PlayPreviousItem() {
    QListWidgetItem* currentItem = ui->Playlist->currentItem();
    if (!currentItem) return;

    int currentRow = ui->Playlist->row(currentItem);
    int previousRow = currentRow - 1;
    if (previousRow < 0) return;

    QListWidgetItem* previousItem = ui->Playlist->item(previousRow);
    QString filename = previousItem->data(Qt::UserRole).toString();  // get full path

    ui->Playlist->setCurrentItem(previousItem);
    start_playback(filename);
}


void Main_PAW_widget::addFilesToPlaylist() {
    QStringList files = QFileDialog::getOpenFileNames(
        nullptr,
        "Open audio files",
        "",
        "Audio Files (*.mp3 *.wav *.flac *.ogg *.opus);;All Files (*)"
    );

    for (const QString &file : files) {
        QByteArray filenameBytes = file.toLocal8Bit();
        const char* filenamechar = filenameBytes.constData();

        QString displayText;

        if (get_metadata(filenamechar, &filemetadata) == 0) {
            QString title = (strlen(filemetadata.title) > 0)
                            ? QString::fromUtf8(filemetadata.title)
                            : QFileInfo(file).fileName(); 
            QString artist = (strlen(filemetadata.artist) > 0)
                             ? QString::fromUtf8(filemetadata.artist)
                             : "";

            displayText = title;
            if (!artist.isEmpty()) {
                displayText += " - " + artist;
            }
        } else {
            displayText = QFileInfo(file).fileName();
        }

        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, file); 
        ui->Playlist->addItem(item);
    }
}

void Main_PAW_widget::handleError(const QString &errorMessage) {
    QMessageBox::critical(this, "Audio Playback Error", errorMessage);
    m_audiothread->stopPlayback(); 
    handlePlaybackFinished(); 
}

void Main_PAW_widget::openSettings(){
    if (s) {
        s->show();
    }
}

void Main_PAW_widget::openAbout(){
    about.show();
}


QString Main_PAW_widget::floatToMMSS(float totalSeconds) {
    bool isNegative = totalSeconds < 0;
    totalSeconds = std::abs(totalSeconds);

    int minutes = static_cast<int>(totalSeconds / 60.0f);
    float remainingSeconds = totalSeconds - (minutes * 60.0f);
    int seconds = static_cast<int>(std::round(remainingSeconds)); 


    if (seconds == 60) {
        seconds = 0;
        minutes++;
    }

    return QStringLiteral("%1%2:%3")
           .arg(isNegative ? "-" : "")
           .arg(minutes, 2, 10, QChar('0'))
           .arg(seconds, 2, 10, QChar('0'));
}