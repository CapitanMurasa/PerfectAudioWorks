#pragma once
#include <QSqlDatabase>
#include "../PAW_GUI/main_paw_widget.h"
#include "file.h"

class Main_PAW_widget;

struct TrackData {
    QString title;
    QString artist;
    QString album;
    QString genre;
    int bitrate;
    QByteArray coverImage;
    bool found = false;
};

class DatabaseManager : public QObject {
    Q_OBJECT
public:

    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    void FillRow(QString path);
    TrackData LoadRow(QString path);

private:
    Main_PAW_widget* mainwidget;

    void createUnifiedSchema();
};