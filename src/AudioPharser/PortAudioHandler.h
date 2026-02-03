#pragma once
#include <QThread>
#include <QString>
#include <QList>
#include <QPair>
#include <QDebug>
#include <QTimer>
#include <cmath>
#include "portaudio_backend.h"

class PortaudioThread : public QThread {
    Q_OBJECT
public:
    explicit PortaudioThread(QObject* parent = nullptr);
    ~PortaudioThread() override;

    void setFile(const QString& filename);
    void setAudioDevice(int device);
    void StartPlayback();
    void stopPlayback();
    void setPlayPause();
    bool isPaused() const;
    void SetFrameFromTimeline(float percent);
    QList<QPair<QString, int>> GetAllAvailableOutputDevices();
    void stop();

signals:
    void errorOccurred(const QString&);
    void playbackProgress(int current, int total, int samplerate);
    void playbackFinished();
    void totalFileInfo(int totalFrames,int channels, int samplerate, const char* codecname);

protected:
    void run() override;

private:
    QString m_filename;
    int audiodevice;
    AudioPlayer m_player;
    bool m_isPaused;
    bool m_isRunning;
    std::atomic<bool> m_stopRequested;

    // Time reference for PortAudio playback
    double m_streamStartTime;

    void emitProgress();
};
