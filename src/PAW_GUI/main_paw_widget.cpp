#include "main_paw_widget.h"
#include "ui_main_paw_widget.h"
#include "aboutfile_paw_gui.h"
#include "Proxy_style.h"
#include <cmath> 
#include <string>
#include <QMenu>        
#include <QAction>      
#include <QMessageBox>  
#include <QFileDialog>  
#include <QFileInfo>
#include <QDirIterator>
#include <QUrl>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

void Main_PAW_widget::SetupUIElements() {
    ui->Playlist->setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true); 
    ui->Playlist->setAcceptDrops(true);

    ui->TimelineSlider->setStyle(new JumpSliderStyle(ui->TimelineSlider->style()));
    ui->VolumeSlider->setStyle(new JumpSliderStyle(ui->VolumeSlider->style()));

    connect(ui->TimelineSlider, &QSlider::valueChanged, this, &Main_PAW_widget::onSliderValueChanged);
    connect(ui->VolumeSlider, &QSlider::valueChanged, this, &Main_PAW_widget::SetVolumeFromSlider);
    ui->VolumeSlider->setValue(100);
    connect(ui->PlayPause, &QPushButton::clicked, this, &Main_PAW_widget::PlayPauseButton);
    connect(ui->Loop, &QPushButton::clicked, this, &Main_PAW_widget::SetLoop);
    connect(ui->actionSettings, &QAction::triggered, this, &Main_PAW_widget::openSettings);
    connect(ui->actionAbout, &QAction::triggered, this, &Main_PAW_widget::openAbout);
    connect(ui->actionadd_files_to_playlist, &QAction::triggered, this, &Main_PAW_widget::addFilesToPlaylist);
    connect(ui->actionadd_folders_to_playlist, &QAction::triggered, this, &Main_PAW_widget::on_actionAddFolder_triggered);
    connect(ui->actionadd_current_file_to_playlist, &QAction::triggered, this, &Main_PAW_widget::addCurrentPlayingfileToPlaylist);
    
    connect(ui->Stop, &QPushButton::clicked, this, &Main_PAW_widget::StopPlayback);
    connect(ui->PreviousTrack, &QPushButton::clicked, this, &Main_PAW_widget::PlayPreviousItem);
    connect(ui->NextTrack, &QPushButton::clicked, this, &Main_PAW_widget::PlayNextItem);
    connect(ui->Playlist, &QListWidget::customContextMenuRequested, this, &Main_PAW_widget::showPlaylistContextMenu);
    connect(ui->Playlist, &QListWidget::itemDoubleClicked, this, &Main_PAW_widget::playSelectedItem);

    connect(m_audiothread, &PortaudioThread::playbackProgress, this, &Main_PAW_widget::handlePlaybackProgress);
    connect(m_audiothread, &PortaudioThread::totalFileInfo, this, &Main_PAW_widget::handleTotalFileInfo);
    connect(m_audiothread, &PortaudioThread::playbackFinished, this, &Main_PAW_widget::handlePlaybackFinished);
    connect(m_audiothread, &PortaudioThread::errorOccurred, this, &Main_PAW_widget::handleError);
}

void Main_PAW_widget::SetupQtActions() {
    m_deleteAction = new QAction("Delete Item", this);
    m_deleteAction->setShortcut(QKeySequence::Delete);
    m_deleteAction->setShortcutContext(Qt::WidgetShortcut);
    connect(m_deleteAction, &QAction::triggered, this, &Main_PAW_widget::deleteSelectedItem);
    ui->Playlist->addAction(m_deleteAction);
}

Main_PAW_widget::Main_PAW_widget(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::Main_PAW_widget)
{
    ui->setupUi(this);

    aboutfile = nullptr;
    currentItemPlaying = nullptr;

    m_audiothread = new PortaudioThread(this);
    s = new Settings_PAW_gui(m_audiothread, this);

    if (!loader.load_jsonfile(settings, "settings.json")) {
        settings = nlohmann::json::object();
        settings["save_playlists"] = true;
        settings["auto_skip_tracks"] = true;
        qDebug() << "Settings not found, using defaults.";
    }

    saveplaylist = settings.value("save_playlists", true);
    CanAutoSwitch = settings.value("auto_skip_tracks", true);

    if (saveplaylist) {
        if (!loader.load_jsonfile(playlist, "playlist.json")) {
            playlist = nlohmann::json::array();
            qDebug() << "Playlist file missing, initialized empty array.";
        }
        else {
            addFilesToPlaylistfromJson();
        }
    }
    else {
        playlist = nlohmann::json::array();
    }

    SetupUIElements();
    SetupQtActions();
    m_updateTimer = new QTimer(this);
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

void Main_PAW_widget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void Main_PAW_widget::dropEvent(QDropEvent* event) {
    const QList<QUrl> urls = event->mimeData()->urls();

    static const QStringList supportedFormats = { "mp3", "wav", "flac", "ogg", "opus", "m4a", "aac"};

    for (const QUrl& url : urls) {
        QString filePath = url.toLocalFile();
        if (filePath.isEmpty()) continue;

        QFileInfo info(filePath);

        if (info.isDir()) {
            addFolderToPlaylist(filePath);
        }
        else {
            QString extension = info.suffix().toLower();

            if (supportedFormats.contains(extension)) {
                ProcessFilesList(filePath);

                if (saveplaylist) {
                    std::string stdPath = filePath.toStdString();
                    if (std::find(playlist.begin(), playlist.end(), stdPath) == playlist.end()) {
                        playlist.push_back(stdPath);
                    }
                }
            }
        }
    }

    if (saveplaylist) {
        loader.save_config(playlist, "playlist.json");
    }
}

void Main_PAW_widget::addFolderToPlaylist(const QString& folderPath) {
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg" << "*.opus" << "*.m4a" << "*.aac";

    QDirIterator it(folderPath, filters, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString filePath = it.next();

        if (saveplaylist) {
            std::string stdPath = filePath.toStdString();
            if (std::find(playlist.begin(), playlist.end(), stdPath) == playlist.end()) {
                playlist.push_back(stdPath);
            }
        }

        ProcessFilesList(filePath);
    }

    if (saveplaylist) {
        loader.save_config(playlist, "playlist.json");
    }
}


void Main_PAW_widget::start_playback(const QString & filename) {
    m_currentFile = filename;

    currentItemPlaying = nullptr;

    QFont normalFont = ui->Playlist->font();
    QFont boldFont = normalFont;
    boldFont.setBold(true);

    for (int i = 0; i < ui->Playlist->count(); ++i) {
        QListWidgetItem* item = ui->Playlist->item(i);

        if (item->data(Qt::UserRole).toString() == filename) {
            currentItemPlaying = item;

            ui->Playlist->setCurrentItem(item);
            item->setFont(boldFont); 
        }
        else {
            item->setFont(normalFont);
        }
    }

    LoadMetadatafromfile();

    if (m_audiothread->isRunning()) {
        m_isSwitching = true;
        disconnect(m_audiothread, &QThread::finished, this, &Main_PAW_widget::startPendingTrack);
        connect(m_audiothread, &QThread::finished, this, &Main_PAW_widget::startPendingTrack);
        m_audiothread->stop();
    }
    else {
        startPendingTrack();
    }
}


void Main_PAW_widget::startPendingTrack() {
    disconnect(m_audiothread, &QThread::finished, this, &Main_PAW_widget::startPendingTrack);

    if (!m_currentFile.isEmpty()) {
        m_audiothread->setFile(m_currentFile);
        m_audiothread->start();

        m_isSwitching = false;
    }
}

void Main_PAW_widget::LoadMetadatafromfile() {
    QString filename = m_currentFile;

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

        QString title = (filemetadata.title && strlen(filemetadata.title) > 0)
            ? QString::fromUtf8(filemetadata.title)
            : filename.section('/', -1);

        QString artist = (filemetadata.artist && strlen(filemetadata.artist) > 0)
            ? QString::fromUtf8(filemetadata.artist)
            : "";

        if (filemetadata.cover_image && filemetadata.cover_size > 0) {
            if (coverArt.loadFromData(filemetadata.cover_image, filemetadata.cover_size)) {
                artFound = true;
            }
        }

        if (filemetadata.bitrate > 0) {
            ui->BitrateInfo->setText(QString::number(filemetadata.bitrate) + " kbps");
        }
        else {
            ui->BitrateInfo->setText("");
        }

        FileInfo_cleanup(&filemetadata);

        m_originalAlbumArt = artFound ? coverArt : QPixmap();
        this->setWindowTitle(artist + " - " + title);
        ui->Filename->setText(title);
        ui->Artist->setText(artist);
    }
    else {
        ui->Filename->setText(filename.section('/', -1));
        ui->Artist->setText("");
        m_originalAlbumArt = QPixmap();
    }
    updateAlbumArt();
}

void Main_PAW_widget::updateAlbumArt()
{
    if (m_originalAlbumArt.isNull()) {
        ui->AlbumArt->setPixmap(QPixmap());
        ui->AlbumArt->hide();
        return;
    }
    else {
        ui->AlbumArt->show();
        QPixmap scaledArt = m_originalAlbumArt.scaled(ui->AlbumArt->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

        ui->AlbumArt->setPixmap(scaledArt);
    }
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

void Main_PAW_widget::handleTotalFileInfo(int totalFrames, int channels, int sampleRate, const char* codecname) {
    if (totalFrames > 0 && sampleRate > 0) {
        float totalDuration = (totalFrames * 1.0f) / sampleRate;
        ui->TotalFileDuration->setText(floatToMMSS(totalDuration));
        ui->SampleRateinfo->setText(QString::number(sampleRate));
        ui->CodecProcessorinfo->setText(QString::fromUtf8(codecname));
        if (channels == 1) {
            ui->ChannelsInfo->setText("Mono");
        }
        else if (channels == 2) {
            ui->ChannelsInfo->setText("Stereo");
        }
    }
}

void Main_PAW_widget::handlePlaybackFinished() {
    ui->TimelineSlider->setValue(0);
    ui->CurrentFileDuration->setText("00:00");

    if (m_isSwitching) {
        return;
    }

    if (ToggleRepeatButton && !m_currentFile.isEmpty()) {
        QTimer::singleShot(0, this, [this]() {
            start_playback(m_currentFile);
            });
    }
    else {
        ClearUi();
        if (CanAutoSwitch) {
            QTimer::singleShot(0, this, &Main_PAW_widget::PlayNextItem);
        }
    }
}

void Main_PAW_widget::on_actionopen_file_triggered() {
    QString filename = QFileDialog::getOpenFileName(this, "Open Audio File", "", "Audio Files (*.wav *.flac *.ogg *.opus *.mp3 *.m4a *.aac);;All Files (*)");
    if (!filename.isEmpty()) {
        start_playback(filename);
    }
}

void Main_PAW_widget::addCurrentPlayingfileToPlaylist() {
    if (m_currentFile.isEmpty()) {
        QMessageBox::information(this, "Info", "No track is currently playing.");
        return;
    }

    if (saveplaylist) {
        std::string stdPath = m_currentFile.toStdString();
        bool alreadyExists = false;

        for (const auto& item : playlist) {
            if (item.get<std::string>() == stdPath) {
                alreadyExists = true;
                break;
            }
        }

        if (alreadyExists) {
            QMessageBox::information(this, "Info", "Track is already in the playlist.");
            return;
        }

        playlist.push_back(stdPath);
        loader.save_config(playlist, "playlist.json");
    }

    ProcessFilesList(m_currentFile);


    if (ui->Playlist->count() > 0) {
        QListWidgetItem* newItem = ui->Playlist->item(ui->Playlist->count() - 1);
        if (newItem->data(Qt::UserRole).toString() == m_currentFile) {
            currentItemPlaying = newItem;

            QFont boldFont = newItem->font();
            boldFont.setBold(true);
            newItem->setFont(boldFont);
            ui->Playlist->setCurrentItem(newItem);
        }
    }
}

void Main_PAW_widget::on_actionAddFolder_triggered() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
        "/home",
        QFileDialog::ShowDirsOnly
        | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        addFolderToPlaylist(dir);
    }
}

void Main_PAW_widget::onSliderValueChanged(float value) {
    if (m_audiothread->isPaused()) {
        m_audiothread->setPlayPause();
        ui->PlayPause->setText("||");
    }
    m_audiothread->SetFrameFromTimeline(value);
}

void Main_PAW_widget::PlayPauseButton() {
    m_audiothread->setPlayPause();
    if (m_audiothread->isPaused()) {
        ui->PlayPause->setText("|>");
    }
    else {
        ui->PlayPause->setText("||");
    }
}

void Main_PAW_widget::StopPlayback() {
    if (m_audiothread->isRunning()) {
        bool oldState = m_audiothread->blockSignals(true);

        ClearUi();
        
        this->setWindowTitle("Perfect Audio Works");

        m_audiothread->stopPlayback();

        m_audiothread->blockSignals(oldState);

        currentItemPlaying = nullptr;
    }
}

void Main_PAW_widget::ClearUi() {
    ui->TimelineSlider->setValue(0);
    ui->CurrentFileDuration->setText("00:00");
    ui->TotalFileDuration->setText("XX:XX");
    ui->Filename->setText("");
    ui->Artist->setText("");
    ui->AlbumArt->hide();
}

void Main_PAW_widget::addFilesToPlaylist() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Open audio files", "", "Audio Files (*.mp3 *.wav *.flac *.ogg *.opus *.m4a *.aac);;All Files (*)");

    for (const QString& file : files) {
        if (saveplaylist) {
            std::string stdPath = file.toStdString();

            if (std::find(playlist.begin(), playlist.end(), stdPath) == playlist.end()) {
                playlist.push_back(stdPath);
            }
        }

        ProcessFilesList(file);
    }

    if (saveplaylist) {
        loader.save_config(playlist, "playlist.json");
    }
}

void Main_PAW_widget::addFilesToPlaylistfromJson() {
    ui->Playlist->clear();

    if (playlist.is_array()) {
        for (const auto& item : playlist) {
            QString file = QString::fromStdString(item.get<std::string>());
            ProcessFilesList(file);
        }
    }
}

void Main_PAW_widget::ProcessFilesList(const QString& file) {
    int metadata_result = -1;
    QString displayText;

#ifdef _WIN32
    std::wstring w_filePath = file.toStdWString();
    metadata_result = get_metadata_w(w_filePath.c_str(), &filemetadata);
#else
    QByteArray utf8_filePath = file.toUtf8();
    metadata_result = get_metadata(utf8_filePath.constData(), &filemetadata);
#endif

    if (metadata_result == 0) {
        QString title = (strlen(filemetadata.title) > 0) ? QString::fromUtf8(filemetadata.title) : QFileInfo(file).fileName();
        QString artist = (strlen(filemetadata.artist) > 0) ? QString::fromUtf8(filemetadata.artist) : "";
        displayText = artist.isEmpty() ? title : title + " - " + artist;
        FileInfo_cleanup(&filemetadata);
    }
    else {
        displayText = QFileInfo(file).fileName();
    }

    QListWidgetItem* item = new QListWidgetItem(displayText);
    item->setData(Qt::UserRole, file);
    ui->Playlist->addItem(item);
}

void Main_PAW_widget::playSelectedItem() {
    QString filename = returnItemPath();
    if (filename.isEmpty()) return;

    start_playback(filename);
}

void Main_PAW_widget::PlayNextItem() {
    if (!currentItemPlaying) {
        if (ui->Playlist->count() > 0) {
            ui->Playlist->setCurrentRow(0);
            playSelectedItem();
        }
        return;
    }

    int currentRow = ui->Playlist->row(currentItemPlaying);
    int nextRow = currentRow + 1;

    if (nextRow >= ui->Playlist->count()) return;

    QListWidgetItem* nextItem = ui->Playlist->item(nextRow);
    QString filename = nextItem->data(Qt::UserRole).toString();

    start_playback(filename);
}

void Main_PAW_widget::PlayPreviousItem() {
    if (!currentItemPlaying) return;

    int currentRow = ui->Playlist->row(currentItemPlaying);
    int previousRow = currentRow - 1;

    if (previousRow < 0) return;

    QListWidgetItem* previousItem = ui->Playlist->item(previousRow);
    QString filename = previousItem->data(Qt::UserRole).toString();

    start_playback(filename);
}

void Main_PAW_widget::deleteSelectedItem()
{
    int currentRow = ui->Playlist->currentRow();

    if (currentRow >= 0)
    {

        QListWidgetItem* itemToDelete = ui->Playlist->item(currentRow);
        if (itemToDelete == currentItemPlaying) {
            currentItemPlaying = nullptr;
            StopPlayback(); 
        }


        if (playlist.is_array() && currentRow < playlist.size()) {
            loader.RemoveItemByIndex(playlist, currentRow);
            loader.save_config(playlist, "playlist.json");
        }

        delete ui->Playlist->takeItem(currentRow);
    }
    else {
        qWarning() << "No item selected to delete.";
    }
}

void Main_PAW_widget::showPlaylistContextMenu(const QPoint& pos) {
    QListWidgetItem* clickedItem = ui->Playlist->itemAt(pos);
    bool itemClicked = (clickedItem != nullptr);

    QMenu contextMenu(this);

    QAction* showDetailsAction = new QAction("Show Details", &contextMenu);
    showDetailsAction->setEnabled(itemClicked);

    connect(showDetailsAction, &QAction::triggered, this, [=]() {
        if (!aboutfile) {
            aboutfile = new Aboutfile_PAW_gui(this);
        }

        if (clickedItem) {
            QString filePath = clickedItem->data(Qt::UserRole).toString();
            aboutfile->setdata(filePath);

            aboutfile->show();
            aboutfile->raise();
            aboutfile->activateWindow();
        }
        });

    contextMenu.addAction(showDetailsAction);

    if (m_deleteAction) {
        contextMenu.addSeparator();
        m_deleteAction->setEnabled(itemClicked);
        contextMenu.addAction(m_deleteAction);
    }

    contextMenu.exec(ui->Playlist->mapToGlobal(pos));
}

void Main_PAW_widget::handleError(const QString& errorMessage) {
    QMessageBox::critical(this, "Audio Playback Error", errorMessage);
    m_audiothread->stopPlayback();
    handlePlaybackFinished();
}

void Main_PAW_widget::SetVolumeFromSlider(int value) {
    float normalizedGain = static_cast<float>(value) / 100.0f;
    m_audiothread->SetGain(normalizedGain);
}

void Main_PAW_widget::openSettings() {
    if (s) {
        s->show();
    }
}

void Main_PAW_widget::openAbout() {
    about.show();
}

void Main_PAW_widget::SetLoop() {
    ToggleRepeatButton = !ToggleRepeatButton;

    qDebug() << "Looping enabled:" << (ToggleRepeatButton ? "true" : "false");

    if (ToggleRepeatButton) {
        ui->Loop->setStyleSheet("color: green; font-weight: bold;");
    }
    else {
        ui->Loop->setStyleSheet("");
    }
}

QString Main_PAW_widget::returnItemPath() {
    QListWidgetItem* item = ui->Playlist->currentItem();
    if (!item) return QString();
    QString filepath = item->data(Qt::UserRole).toString();
    return filepath;
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