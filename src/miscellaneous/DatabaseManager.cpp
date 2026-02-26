#include "DatabaseManager.h"
#include "../PAW_GUI/main_paw_widget.h"
#include <QSqlDatabase>
#include <QSqlQuery>   
#include <QSqlError>

static bool databaseFileExists(const QString& dbName) {
	return QFile::exists(dbName);
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent)
{
    mainwidget = qobject_cast<Main_PAW_widget*>(parent);


    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) dir.mkpath(".");
    QString dbPath = dir.filePath("library.db");


    if (!QSqlDatabase::contains("PAW_CONNECTION")) {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "PAW_CONNECTION");
        db.setDatabaseName(dbPath);

        if (db.open()) {
            createUnifiedSchema();
        }
        else {
            qCritical() << "SQL Open Error:" << db.lastError().text();
        }
    }
}

DatabaseManager::~DatabaseManager() {
    {
        QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
        if (db.isOpen()) db.close();
    }
    QSqlDatabase::removeDatabase("PAW_CONNECTION");
}

void DatabaseManager::createUnifiedSchema() {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");

    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);

    query.exec("PRAGMA foreign_keys = ON;");

    query.exec("CREATE TABLE IF NOT EXISTS formats ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS artists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS genres ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS albums ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "title TEXT UNIQUE NOT NULL, "
        "cover_image BLOB)");

    query.exec("CREATE TABLE IF NOT EXISTS tracks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "path TEXT UNIQUE NOT NULL, "
        "title TEXT, "
        "bitrate INTEGER, "
        "artist_id INTEGER, "
        "genre_id INTEGER, "
        "album_id INTEGER, "
        "format_id INTEGER, "
        "duration_s INTEGER, "
        "FOREIGN KEY(artist_id) REFERENCES artists(id), "
        "FOREIGN KEY(genre_id) REFERENCES genres(id), "
        "FOREIGN KEY(album_id) REFERENCES albums(id),"
        "FOREIGN KEY(format_id) REFERENCES formats(id)) ");

    query.exec("CREATE TABLE IF NOT EXISTS playlists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS playlist_items ("
        "playlist_id INTEGER, "
        "track_id INTEGER, "
        "PRIMARY KEY(playlist_id, track_id), "
        "FOREIGN KEY(playlist_id) REFERENCES playlists(id) ON DELETE CASCADE, "
        "FOREIGN KEY(track_id) REFERENCES tracks(id) ON DELETE CASCADE)");

    if (!db.commit()) {
        qCritical() << "Failed to commit schema to disk:" << db.lastError().text();
        db.rollback(); 
    }
    else {
        qDebug() << "Database schema committed and ready for FillRow!";
    }
}

void DatabaseManager::FillRow(FileInfo file, QString path) {
    QString targetAlbumTitle = QString::fromStdString(file.album == "" ? file.title : file.album);
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return;
    }

    QByteArray coverBlob;
    if (file.cover_image && file.cover_size > 0) {
        coverBlob = QByteArray(reinterpret_cast<const char*>(file.cover_image), file.cover_size);
    }

    QSqlQuery query(db);

    query.prepare("INSERT OR IGNORE INTO formats (name) VALUES (:name)");
    query.bindValue(":name", QString::fromStdString(file.format));
    query.exec();

    query.prepare("SELECT id FROM formats WHERE name = :name");
    query.bindValue(":name", QString::fromStdString(file.format));
    query.exec();
    int formatId = -1;
    if (query.next()) formatId = query.value(0).toInt();

    query.prepare("INSERT OR IGNORE INTO genres (name) VALUES (:name)");
    query.bindValue(":name", QString::fromStdString(file.genre));
    query.exec();

    query.prepare("SELECT id FROM genres WHERE name = :name");
    query.bindValue(":name", QString::fromStdString(file.genre));
    query.exec();
    int genreId = -1;
    if (query.next()) genreId = query.value(0).toInt();

    query.prepare("INSERT OR IGNORE INTO artists (name) VALUES (:name)");
    query.bindValue(":name", QString::fromStdString(file.artist));
    query.exec();

    query.prepare("SELECT id FROM artists WHERE name = :name");
    query.bindValue(":name", QString::fromStdString(file.artist));
    query.exec();
    int artistId = -1;
    if (query.next()) artistId = query.value(0).toInt();

    query.prepare("INSERT OR IGNORE INTO albums (title, cover_image) VALUES (:title, :cover)");
    query.bindValue(":title", targetAlbumTitle);


    if (coverBlob.isEmpty()) {
        query.bindValue(":cover", QVariant(QMetaType::fromType<QByteArray>()));
    }
    else {
        query.bindValue(":cover", coverBlob);
    }
    query.exec();

    query.prepare("SELECT id FROM albums WHERE title = :title");
    query.bindValue(":title", targetAlbumTitle);
    query.exec();
    int albumId = -1;
    if (query.next()) albumId = query.value(0).toInt();
    qDebug() << albumId;

    
    query.prepare("INSERT OR REPLACE INTO tracks (path, title, bitrate, genre_id, artist_id, album_id, format_id, duration_s) "
        "VALUES (:path, :title, :bitrate, :genre_id, :artist_id, :album_id, :format_id, :duration_s)");
    query.bindValue(":path", path);
    query.bindValue(":title", QString::fromStdString(file.title));
    query.bindValue(":genre_id", genreId);
    query.bindValue(":bitrate",  file.bitrate);
    query.bindValue(":artist_id", artistId);
    query.bindValue(":album_id", albumId);
    query.bindValue(":format_id", formatId);
    query.bindValue(":duration_s", file.durationSeconds);

    if (!query.exec()) {
        qCritical() << "Failed to insert track:" << query.lastError().text();
    }

    if (!db.commit()) {
        qCritical() << "Failed to commit track data to disk:" << db.lastError().text();
        db.rollback();
    }
    else {
        qDebug() << "Track inserted/updated successfully!";
    }

    FileInfo_cleanup(&file);
}

void DatabaseManager::InflatePlaylist(QString path, int requestedId) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) return;

    QSqlQuery query(db);

    int trackDbId = -1;
    query.prepare("SELECT id FROM tracks WHERE path = :path");
    query.bindValue(":path", path);
    if (query.exec() && query.next()) {
        trackDbId = query.value(0).toInt();
    }

    if (trackDbId == -1) {
        db.rollback();
        return;
    }

    int finalPlaylistId = -1;

    query.prepare("SELECT id FROM playlists WHERE id = :id");
    query.bindValue(":id", requestedId);

    if (query.exec() && query.next()) {
        finalPlaylistId = query.value(0).toInt();
    }
    else {
        if (query.exec("SELECT id FROM playlists LIMIT 1") && query.next()) {
            finalPlaylistId = query.value(0).toInt();
        }
        else {
            query.exec("INSERT INTO playlists (name) VALUES ('Default Playlist')");
            finalPlaylistId = query.lastInsertId().toInt();
        }
    }

    query.prepare("INSERT OR REPLACE INTO playlist_items (playlist_id, track_id) "
        "VALUES (:pid, :tid)");
    query.bindValue(":pid", finalPlaylistId);
    query.bindValue(":tid", trackDbId);

    if (query.exec() && db.commit()) {
        qDebug() << "Track" << trackDbId << "added to Playlist" << finalPlaylistId;
    }
    else {
        db.rollback();
    }
}

QList<TrackData> DatabaseManager::LoadPlaylist(int playlistid) {
    QList<TrackData> tracklist;
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    QSqlQuery query(db);

    query.prepare("SELECT t.id, t.path, t.title, a.name, t.duration_s "
        "FROM tracks t "
        "INNER JOIN playlist_items pi ON t.id = pi.track_id "
        "LEFT JOIN artists a ON t.artist_id = a.id "
        "WHERE pi.playlist_id = :pid "
        "ORDER BY pi.track_id ASC");

    query.bindValue(":pid", playlistid);

    if (query.exec()) {
        while (query.next()) {
            TrackData data;
            data.id = query.value(0).toInt();
            data.path = query.value(1).toString();
            data.title = query.value(2).toString();
            data.artist = query.value(3).toString();

            data.duration = query.value(4).toInt();

            tracklist.append(data);
        }
    }
    else {
        qCritical() << "LoadPlaylist failed:" << query.lastError().text();
    }

    return tracklist;
}

void DatabaseManager::AddPlaylist(QString playlistname) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);

    query.prepare("INSERT or IGNORE INTO playlists (name) VALUES (:name)");
    query.bindValue(":name", playlistname);
    query.exec();


    if (!db.commit()) {
        db.rollback();
    }
    else {
        qDebug() << "Playlist Added";
    }
}

QString DatabaseManager::FetchPlaylist(int id) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");

    QSqlQuery query(db);

    query.prepare("SELECT name FROM playlists WHERE id = (:id)");
    query.bindValue(":id", id);
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }

    return QString();
}

QList<Playlistdata> DatabaseManager::FetchPlaylists() {
    QList<Playlistdata> list;
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    QSqlQuery query("SELECT id, name FROM playlists", db);

    while (query.next()) {
        Playlistdata data;
        data.id = query.value(0).toInt();
        data.Name = query.value(1).toString();
        list.append(data);
    }
    return list;
}

void DatabaseManager::RemoveFromPlaylist(QString path, int playlistId) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    QSqlQuery query(db);

    int trackDbId = -1;
    query.prepare("SELECT id FROM tracks WHERE path = :path");
    query.bindValue(":path", path);
    if (query.exec() && query.next()) {
        trackDbId = query.value(0).toInt();
    }

    if (trackDbId != -1) {
        query.prepare("DELETE FROM playlist_items WHERE playlist_id = :pid AND track_id = :tid");
        query.bindValue(":pid", playlistId);
        query.bindValue(":tid", trackDbId);

        if (!query.exec()) {
            qCritical() << "Failed to remove item from playlist:" << query.lastError().text();
        }
        else {
            qDebug() << "Track removed from database playlist successfully.";
        }
    }
}

bool DatabaseManager::TrackExists(QString path) {
    QSqlQuery query(QSqlDatabase::database("PAW_CONNECTION"));
    query.prepare("SELECT id from tracks WHERE path = :path");
    query.bindValue(":path", path);
    return (query.exec() && query.next());
}

TrackData DatabaseManager::LoadRow(QString path) {
    TrackData data;
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");

    QSqlQuery query(db);

    query.prepare("SELECT * from tracks WHERE path = :path");
    query.bindValue(":path", path);

    if (query.exec() && query.next()) {
        data.found = true;

        data.path = query.value(1).toString();
        data.title = query.value(2).toString();
        data.bitrate = query.value(3).toInt();
        data.duration = query.value(8).toInt();
        int artistId = query.value(4).toInt();
        int genreId = query.value(5).toInt();
        int albumId = query.value(6).toInt();
        int formatId = query.value(7).toInt();

        query.prepare("SELECT * from artists WHERE id = :artist");
        query.bindValue(":artist", artistId);
        if (query.exec() && query.next()) { 
            data.artist = query.value(1).toString();
        }

        query.prepare("SELECT * from albums WHERE id = :album");
        query.bindValue(":album", albumId);
        if (query.exec() && query.next()) {
            data.album = query.value(1).toString();
            data.coverImage = query.value(2).toByteArray();
        }

        query.prepare("SELECT * from genres WHERE id = :genre");
        query.bindValue(":genre", genreId);
        if (query.exec() && query.next()) {
            data.genre = query.value(1).toString();
        }

        query.prepare("SELECT * from formats WHERE id = :format");
        query.bindValue(":format", formatId);
        if (query.exec() && query.next()) {
            data.format = query.value(1).toString();
        }

        qDebug() << "Title:" << data.title;
        qDebug() << "Artist:" << data.artist;
        qDebug() << "Album:" << data.album;
        qDebug() << "Genre:" << data.genre;
        qDebug() << "format:" << data.format;
    }
    else {
        qDebug() << "Track not found in database for path:" << path;
    }

    return data;
}