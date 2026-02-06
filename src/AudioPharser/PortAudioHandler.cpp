#include "PortAudioHandler.h"
#include "../miscellaneous/misc.h"
#include <cmath> 

PortaudioThread::PortaudioThread(QObject* parent)
    : QThread(parent)
    , audiodevice(-1)
    , m_isPaused(false)
    , m_isRunning(false)
    , m_streamStartTime(0.0)
    , m_stopRequested(false)
{
    audio_init();

    memset(&m_player, 0, sizeof(m_player));


    pa_mutex_init(&m_player.lock);
}

PortaudioThread::~PortaudioThread() {
    stopPlayback();

    audio_terminate();

    pa_mutex_destroy(&m_player.lock);
}

void PortaudioThread::setFile(const QString& filename) {
    m_filename = filename;
}

void PortaudioThread::setAudioDevice(int device) {
    audiodevice = device;
}

void PortaudioThread::SetGain(float gain) {
    m_player.gain = clamp_float(gain, 0.0f, 1.0f);
}

void PortaudioThread::stop() {
    m_stopRequested = true;
}

void PortaudioThread::stopPlayback() {
    m_isRunning = false;
    m_stopRequested = true;
    wait();
}

void PortaudioThread::setPlayPause() {
    m_isPaused = !m_isPaused;

    audio_pause(&m_player, m_isPaused ? 1 : 0);

    if (!m_isPaused) {
        pa_mutex_lock(&m_player.lock);
        if (m_player.stream && Pa_IsStreamActive(m_player.stream) == 1) {
            m_streamStartTime = Pa_GetStreamTime(m_player.stream) -
                ((double)m_player.currentFrame / m_player.samplerate);
        }
        pa_mutex_unlock(&m_player.lock);
    }
}

bool PortaudioThread::isPaused() const {
    return m_isPaused;
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

    if (m_player.stream) {
        m_streamStartTime = Pa_GetStreamTime(m_player.stream);
    }

    emit totalFileInfo((int)m_player.totalFrames, m_player.channels, m_player.samplerate, m_player.CodecName);
}

void PortaudioThread::run() {
    m_stopRequested = false;
    StartPlayback();

    while (m_isRunning) {
        if (m_stopRequested) break;

        pa_mutex_lock(&m_player.lock);

        if (m_player.stream) {
            if (Pa_IsStreamActive(m_player.stream) <= 0 && !m_isPaused) {
                pa_mutex_unlock(&m_player.lock);
                break; // Exit loop
            }

            if (!m_isPaused && m_player.codec) {
                double streamTime = Pa_GetStreamTime(m_player.stream) - m_streamStartTime;
                long calculatedFrame = static_cast<long>(std::round(streamTime * m_player.samplerate));

                m_player.currentFrame = calculatedFrame;
                emitProgress();
            }
        }

        pa_mutex_unlock(&m_player.lock);

        QThread::msleep(10);
    }

    emitProgress();

    audio_stop(&m_player);

    m_isRunning = false;
    emit playbackFinished();
}

void PortaudioThread::changeAudioDevice(int newDeviceID) {
    audiodevice = newDeviceID;

    if (m_isRunning) {
        int result = device_hotswap(&m_player, audiodevice);

        if (result == 0) {
            pa_mutex_lock(&m_player.lock);
            if (m_player.stream) {
                m_streamStartTime = Pa_GetStreamTime(m_player.stream) -
                    ((double)m_player.currentFrame / m_player.samplerate);
            }
            pa_mutex_unlock(&m_player.lock);
        }
        else {
            emit errorOccurred("Failed to swap audio device");
        }
    }
}

void PortaudioThread::SetFrameFromTimeline(float percent) {
    if (!m_player.codec) return;

    int64_t targetFrame = (int64_t)(m_player.totalFrames * percent / 100.0f);

    audio_seek(&m_player, targetFrame);

    pa_mutex_lock(&m_player.lock);
    if (m_player.stream) {
        m_streamStartTime = Pa_GetStreamTime(m_player.stream) -
            ((double)m_player.currentFrame / m_player.samplerate);
    }
    pa_mutex_unlock(&m_player.lock);

    emitProgress();
}

void PortaudioThread::emitProgress() {
    emit playbackProgress((int)m_player.currentFrame, (int)m_player.totalFrames, m_player.samplerate);
}

QList<QPair<QString, int>> PortaudioThread::GetAllAvailableOutputDevices() {
    QList<QPair<QString, int>> deviceList;

    Pa_Terminate();
    Pa_Initialize();

    int quantityDevices = Pa_GetDeviceCount();
    for (int i = 0; i < quantityDevices; ++i) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
        if (deviceInfo && deviceInfo->maxOutputChannels > 0) {
            deviceList.append(qMakePair(QString::fromUtf8(deviceInfo->name), i));
        }
    }
    return deviceList;
}