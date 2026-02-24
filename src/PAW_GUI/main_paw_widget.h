#ifndef MAIN_PAW_WIDGET_H
#define MAIN_PAW_WIDGET_H

#include <QMainWindow>

#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QMessageBox>
#include <QAction>
#include <QSystemTrayIcon>

#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/file.h" 
#include "../miscellaneous/json.h"
#include "../miscellaneous/DatabaseManager.h"

#include "settings_paw_gui.h"
#include "about_paw_gui.h"
#include "aboutfile_paw_gui.h"
#include "playlist_paw_manager.h"
#include "loadingplaylists.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Main_PAW_widget;
}
QT_END_NAMESPACE

class Settings_PAW_gui;
class DatabaseManager;
class Playlist_Paw_Manager;
class loadingplaylists;
class QTableWidgetItem;

class Main_PAW_widget : public QMainWindow
{
    Q_OBJECT

public:
    explicit Main_PAW_widget(QWidget *parent = nullptr); 
    ~Main_PAW_widget() override; 
    void start_playback(const QString &filename);
    void PlayPreviousItem();
    void PlayNextItem();
    void StopPlayback();
    void PlayPauseButton();
    void addFilesToPlaylistfromDatabase(int id);

    FileInfo file_info_current;

    QString returnTimeElapsed();
    QString returnTimeStamp();
    QString m_currentFile;
    int CurrentPlaylistId = 1;

    bool CanAutoSwitch = true;

    
    PortaudioThread& getAudioThread() { return *m_audiothread; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
   
    void on_actionopen_file_triggered();
    void onSliderValueChanged(float value);
    void addFilesToPlaylist();
    void showPlaylistContextMenu(const QPoint &pos);
    void playSelectedItem();
    void deleteSelectedItem();
    void SetLoop();
    void SetVolumeFromSlider(int value);
    void addCurrentPlayingfileToPlaylist();
    void showAboutTrackinfo();
    
    void handlePlaybackProgress(int currentFrame, int totalFrames, int sampleRate);
    void handleTotalFileInfo(int totalFrames,int channels, int sampleRate, const char* codecname);
    void handlePlaybackFinished();
    void handleError(const QString &errorMessage);
    void openSettings();
    void on_actionAddFolder_triggered();
    void openAbout();

private:

    QTimer *m_updateTimer; 
    QAction *m_deleteAction;
    PortaudioThread* m_audiothread; 
    FileInfo filemetadata;
    QTableWidgetItem* currentItemPlaying;
    QPixmap m_originalAlbumArt;      
    Settings_PAW_gui *s;
    Aboutfile_PAW_gui *aboutfile;
    About_PAW_gui *about;
    JsonLoader loader;
    QSystemTrayIcon *trayIcon;
    DatabaseManager *database;
    Playlist_Paw_Manager *playlistmanager;
    loadingplaylists *loadtoplaylistbar;
    float currentDuration;
    float totalDuration;

    QString floatToMMSS(float totalSeconds);
    void updateAlbumArt();
    void SetupUIElements();
    void startPendingTrack();
    void SetupQtActions();
    void ProcessFilesToPlaylist(QStringList files);
    void launchPlaylistManager();
    void LoadMetadatafromfile();
    QString returnItemPath();
    void ProcessFilesList(const QString& file);
    void ClearUi();

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void addFolderToPlaylist(const QString& folderPath);

    bool finished_playing;
    bool ToggleRepeatButton = false;
    bool m_isSwitching = false;
    bool saveplaylist;

    json settings;

    Ui::Main_PAW_widget *ui; 
};
#endif 
