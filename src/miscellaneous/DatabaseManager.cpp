#include "DatabaseManager.h"
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
        "FOREIGN KEY(artist_id) REFERENCES artists(id), "
        "FOREIGN KEY(genre_id) REFERENCES genres(id), "
        "FOREIGN KEY(album_id) REFERENCES albums(id))");

    query.exec("CREATE TABLE IF NOT EXISTS playlists ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT UNIQUE NOT NULL)");

    query.exec("CREATE TABLE IF NOT EXISTS playlist_items ("
        "playlist_id INTEGER, "
        "track_id INTEGER, "
        "position INTEGER, "
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
    query.bindValue(":title", QString::fromStdString(file.album));

    if (coverBlob.isEmpty()) {
        query.bindValue(":cover", QVariant(QMetaType::fromType<QByteArray>()));
    }
    else {
        query.bindValue(":cover", coverBlob);
    }
    query.exec();

    query.prepare("SELECT id FROM albums WHERE title = :title");
    query.bindValue(":title", QString::fromStdString(file.album));
    query.exec();
    int albumId = -1;
    if (query.next()) albumId = query.value(0).toInt();
    qDebug() << albumId;

    
    query.prepare("INSERT OR REPLACE INTO tracks (path, title, bitrate, genre_id, artist_id, album_id) "
        "VALUES (:path, :title, :bitrate, :genre_id, :artist_id, :album_id)");
    query.bindValue(":path", path);
    query.bindValue(":title", QString::fromStdString(file.title));
    query.bindValue(":genre_id", genreId);
    query.bindValue(":bitrate",  file.bitrate);
    query.bindValue(":artist_id", artistId);
    query.bindValue(":album_id", albumId);

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

void DatabaseManager::InflatePlaylist(QString path, QString playlistName) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return;
    }

    QSqlQuery query(db);

    int TrackId = -1;
    query.prepare("SELECT id from tracks WHERE path = :path");
    query.bindValue(":path", path);
    if (query.exec() && query.next()) {
        TrackId = query.value(0).toInt();
    }

    if (TrackId == -1) {
        qCritical() << "Cannot add track to playlist: Track not in database.";
        db.rollback();
        return;
    }

    query.prepare("INSERT OR IGNORE INTO playlists (name) VALUES (:name)");
    query.bindValue(":name", playlistName);
    query.exec();

    query.prepare("SELECT id FROM playlists WHERE name = :name");
    query.bindValue(":name", playlistName);
    query.exec();
    int playlistId = -1;
    if (query.next()) playlistId = query.value(0).toInt();

    query.prepare("INSERT OR REPLACE INTO playlist_items (playlist_id, track_id) VALUES (:playlist_id, :track_id)");
    query.bindValue(":playlist_id", playlistId);
    query.bindValue(":track_id", TrackId);

    if (!query.exec()) {
        qCritical() << "Failed to add to playlist_items:" << query.lastError().text();
    }

    if (!db.commit()) {
        db.rollback();
    }
    else {
        qDebug() << "Track added to playlist successfully!";
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

        data.title = query.value(2).toString();
        data.bitrate = query.value(3).toInt();
        int artistId = query.value(4).toInt();
        int genreId = query.value(5).toInt();
        int albumId = query.value(6).toInt();

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

        qDebug() << "Title:" << data.title;
        qDebug() << "Artist:" << data.artist;
        qDebug() << "Album:" << data.album;
        qDebug() << "Genre:" << data.genre;
    }
    else {
        qDebug() << "Track not found in database for path:" << path;
    }

    return data;
}