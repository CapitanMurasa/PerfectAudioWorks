#include "PortAudioHandler.h"
#include <string>

PortaudioThread::PortaudioThread(QObject* parent)
    : QThread(parent),
      audiodevice(-1),
      m_isPaused(false),
      m_isRunning(false),
      m_streamStartTime(0.0)
{
    m_stopRequested = false;
    audio_init();
    memset(&m_player, 0, sizeof(m_player));
}

PortaudioThread::~PortaudioThread() {
    stopPlayback();
    audio_terminate();
}

void PortaudioThread::stop()
{
    m_stopRequested = true; 
}

QList<QPair<QString, int>> PortaudioThread::GetAllAvailableOutputDevices() {
    QList<QPair<QString, int>> deviceList;
    int quantityDevices = Pa_GetDeviceCount();

    for (int i = 0; i < quantityDevices; ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && deviceInfo->maxOutputChannels > 0) {
            deviceList.append(qMakePair(QString(deviceInfo->name), i));
        }
    }
    return deviceList;
}

void PortaudioThread::setFile(const QString& filename) {
    m_filename = filename;
}

void PortaudioThread::setAudioDevice(int device) {
    audiodevice = device;
}

void PortaudioThread::StartPlayback() {
    if (m_filename.isEmpty()) return;

    int play_result = -1;
#ifdef _WIN32
    std::wstring w_filename = m_filename.toStdWString();
    play_result = audio_play_w(&m_player, w_filename.c_str(), audiodevice);
#else
    QByteArray utf8_filename = m_filename.toUtf8();
    play_result = audio_play(&m_player, utf8_filename.constData(), audiodevice);
#endif

    if (play_result != 0) {
        emit errorOccurred("Failed to start playback");
        return;
    }

    m_isRunning = true;
    m_streamStartTime = Pa_GetStreamTime(m_player.stream) - ((double)m_player.currentFrame / m_player.samplerate);

    emit totalFileInfo((int)m_player.totalFrames, m_player.channels, m_player.samplerate, m_player.CodecName);
}

void PortaudioThread::run() {
    m_stopRequested = false;
    StartPlayback();

    while (m_player.stream && Pa_IsStreamActive(m_player.stream) && m_isRunning) {
        if (m_stopRequested) {
            break; 
        }
        if (!m_isPaused && m_player.codec) {
            double streamTime = Pa_GetStreamTime(m_player.stream) - m_streamStartTime;
            long frameFromTime = static_cast<long>(std::round(streamTime * m_player.samplerate));

            if (frameFromTime != m_player.currentFrame) {
                m_player.currentFrame = frameFromTime;
                emitProgress();
            }
        }

        QThread::msleep(10);
    }

    emitProgress();
    
    audio_stop(&m_player); 

    emit playbackFinished();
    
    m_isRunning = false;
}

void PortaudioThread::stopPlayback() {
    m_isRunning = false;
    wait();
}

void PortaudioThread::setPlayPause() {
    m_isPaused = !m_isPaused;
    audio_pause(&m_player, m_isPaused ? 1 : 0);

    if (!m_isPaused && m_player.codec && m_player.stream) {
        m_streamStartTime = Pa_GetStreamTime(m_player.stream) - ((double)m_player.currentFrame / m_player.samplerate);
    }
}

bool PortaudioThread::isPaused() const {
    return m_isPaused;
}

void PortaudioThread::SetFrameFromTimeline(float percent) {
    if (!m_player.codec || !m_player.stream) return;

    int64_t frame = (m_player.totalFrames * percent) / 100;

    audio_seek(&m_player, frame);

    m_streamStartTime = Pa_GetStreamTime(m_player.stream) - (double)frame / m_player.samplerate;

    m_player.currentFrame = frame;

    emitProgress();
}

void PortaudioThread::emitProgress() {
    if (!m_player.codec) return;
    emit playbackProgress((int)m_player.currentFrame, (int)m_player.totalFrames, m_player.samplerate);
}