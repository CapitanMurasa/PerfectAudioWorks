#pragma once
#ifdef Q_OS_LINUX
#include <QObject>
#include <QtDBus>

class LinuxKeys : public QDBusAbstractAdaptor {
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

public:
    explicit LinuxKeys(QObject* parent) : QDBusAbstractAdaptor(parent) {
        setAutoRelaySignals(true);
    }

    Q_PROPERTY(bool CanControl READ canControl)
        Q_PROPERTY(bool CanGoNext READ canControl)
        Q_PROPERTY(bool CanGoPrevious READ canControl)
        Q_PROPERTY(bool CanPlay READ canControl)
        Q_PROPERTY(bool CanPause READ canControl)

        bool canControl() const { return true; }

public slots:
    void PlayPause() { emit playPauseRequested(); }
    void Next() { emit nextRequested(); }
    void Previous() { emit previousRequested(); }
    void Stop() { emit stopRequested(); }

    void OpenUri(QString) {}
    void Seek(qlonglong) {}
    void SetPosition(QDBusObjectPath, qlonglong) {}

signals:
    void playPauseRequested();
    void nextRequested();
    void previousRequested();
    void stopRequested();
};
#endif