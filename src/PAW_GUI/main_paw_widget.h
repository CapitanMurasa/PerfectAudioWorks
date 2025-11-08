#ifndef MAIN_PAW_WIDGET_H
#define MAIN_PAW_WIDGET_H

#include <QMainWindow>

#include <QFileDialog>
#include <QTimer>
#include <QDebug>
#include <QMessageBox> 

#include "../AudioPharser/PortAudioHandler.h" 
#include "../miscellaneous/file.h" 

#include "settings_paw_gui.h"
#include "about_paw_gui.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Main_PAW_widget;
}
QT_END_NAMESPACE


class Main_PAW_widget : public QMainWindow
{
    Q_OBJECT

public:
    explicit Main_PAW_widget(QWidget *parent = nullptr); 
    ~Main_PAW_widget() override; 


    void start_playback(const QString &filename);

    
    PortaudioThread& getAudioThread() { return *m_audiothread; }

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
   
    void on_actionopen_file_triggered();
    void onSliderValueChanged(float value);
    void PlayPauseButton();
    void addFilesToPlaylist();
    void StopPlayback();
    void PlayPreviousItem();
    void PlayNextItem();
    
    void handlePlaybackProgress(int currentFrame, int totalFrames, int sampleRate);
    void handleTotalFileInfo(int totalFrames,int channels, int sampleRate, const char* codecname);
    void handlePlaybackFinished();
    void handleError(const QString &errorMessage);
    void openSettings();
    void openAbout();

private:

    QTimer *m_updateTimer; 
    PortaudioThread* m_audiothread; 
    QString m_currentFile; 
    FileInfo filemetadata;
    QPixmap m_originalAlbumArt;      
    Settings_PAW_gui *s;
    About_PAW_gui about;

    QString floatToMMSS(float totalSeconds);
    void updateAlbumArt();

    bool finished_playing;

    Ui::Main_PAW_widget *ui; 
};
#endif 