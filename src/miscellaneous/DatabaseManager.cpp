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
        db.rollback();
    }
}

QList<TrackData> DatabaseManager::IndexResult(QString request, int playlist_id) {
    QList<TrackData> tracklist;
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    QSqlQuery query(db);

    query.prepare("SELECT t.id, t.path, t.title, a.name, t.duration_s "
    "FROM tracks t "
    "LEFT JOIN artists a ON t.artist_id = a.id "
    "WHERE t.title LIKE :value "
    "AND t.id IN (SELECT track_id FROM playlist_items WHERE playlist_id = :pid)");
    query.bindValue(":value", "%" + request + "%");
    query.bindValue(":pid", playlist_id);

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

    return tracklist;
}

void DatabaseManager::FillRow(FileInfo file, QString path) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        return;
    }

    QString targetAlbum  = QString::fromStdString(file.album).trimmed();
    QString targetTitle  = QString::fromStdString(file.title).trimmed();
    QString targetArtist = QString::fromStdString(file.artist).trimmed();
    QString targetGenre  = QString::fromStdString(file.genre).trimmed();
    QString targetFormat = QString::fromStdString(file.format).trimmed();

    if (targetAlbum.isEmpty()) {
        targetAlbum = targetTitle.isEmpty() ? "Unknown Album" : targetTitle;
    }
    if (targetArtist.isEmpty()) targetArtist = "Unknown Artist";
    if (targetGenre.isEmpty())  targetGenre  = "Unknown Genre";

    QByteArray coverBlob;
    if (file.cover_image && file.cover_size > 0) {
        coverBlob = QByteArray(reinterpret_cast<const char*>(file.cover_image), file.cover_size);
    }

    QSqlQuery query(db);

    // Upsert lookup table and return ID in a single query
    auto getOrInsertId = [&](const QString& table, const QString& val) -> int {
        query.prepare(QString("INSERT INTO %1 (name) VALUES (:val) "
                              "ON CONFLICT(name) DO UPDATE SET name=excluded.name "
                              "RETURNING id").arg(table));
        query.bindValue(":val", val);
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        return -1;
    };

    int formatId = getOrInsertId("formats", targetFormat);
    int genreId  = getOrInsertId("genres", targetGenre);
    int artistId = getOrInsertId("artists", targetArtist);

    // Upsert album and return ID
    query.prepare("INSERT INTO albums (title, cover_image) VALUES (:title, :cover) "
                  "ON CONFLICT(title) DO UPDATE SET cover_image = COALESCE(excluded.cover_image, albums.cover_image) "
                  "RETURNING id");
    query.bindValue(":title", targetAlbum);
    if (coverBlob.isEmpty()) {
        query.bindValue(":cover", QVariant(QMetaType::fromType<QByteArray>()));
    } else {
        query.bindValue(":cover", coverBlob);
    }

    int albumId = -1;
    if (query.exec() && query.next()) {
        albumId = query.value(0).toInt();
    }

    // Upsert track entry
    query.prepare("INSERT INTO tracks (path, title, bitrate, genre_id, artist_id, album_id, format_id, duration_s) "
                  "VALUES (:path, :title, :bitrate, :genre_id, :artist_id, :album_id, :format_id, :duration_s) "
                  "ON CONFLICT(path) DO UPDATE SET "
                  "title=excluded.title, "
                  "bitrate=excluded.bitrate, "
                  "genre_id=excluded.genre_id, "
                  "artist_id=excluded.artist_id, "
                  "album_id=excluded.album_id, "
                  "format_id=excluded.format_id, "
                  "duration_s=excluded.duration_s");

    query.bindValue(":path", path);
    query.bindValue(":title", QString::fromStdString(file.title));
    query.bindValue(":genre_id", genreId);
    query.bindValue(":bitrate", file.bitrate);
    query.bindValue(":artist_id", artistId);
    query.bindValue(":album_id", albumId);
    query.bindValue(":format_id", formatId);
    query.bindValue(":duration_s", file.durationSeconds);

    if (!query.exec()) {
        db.rollback();
    } else if (!db.commit()) {
        db.rollback();
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
    } else {
        if (query.exec("SELECT id FROM playlists LIMIT 1") && query.next()) {
            finalPlaylistId = query.value(0).toInt();
        } else {
            query.exec("INSERT INTO playlists (name) VALUES ('Default Playlist')");
            finalPlaylistId = query.lastInsertId().toInt();
        }
    }

    query.prepare("INSERT OR REPLACE INTO playlist_items (playlist_id, track_id) "
    "VALUES (:pid, :tid)");
    query.bindValue(":pid", finalPlaylistId);
    query.bindValue(":tid", trackDbId);

    if (!(query.exec() && db.commit())) {
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

    return tracklist;
}

void DatabaseManager::AddPlaylist(QString playlistname) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) return;

    QSqlQuery query(db);
    query.prepare("INSERT or IGNORE INTO playlists (name) VALUES (:name)");
    query.bindValue(":name", playlistname);

    if (query.exec()) {
        db.commit();
    } else {
        db.rollback();
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
        query.exec();
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

    query.prepare(
        "SELECT t.path, t.title, t.bitrate, t.duration_s, "
        "       a.name AS artist, "
        "       al.title AS album, al.cover_image, "
        "       g.name AS genre, "
        "       f.name AS format "
        "FROM tracks t "
        "LEFT JOIN artists a  ON t.artist_id = a.id "
        "LEFT JOIN albums al  ON t.album_id  = al.id "
        "LEFT JOIN genres g   ON t.genre_id  = g.id "
        "LEFT JOIN formats f  ON t.format_id = f.id "
        "WHERE t.path = :path"
    );
    query.bindValue(":path", path);

    if (query.exec() && query.next()) {
        data.found      = true;
        data.path       = query.value(0).toString();
        data.title      = query.value(1).toString();
        data.bitrate    = query.value(2).toInt();
        data.duration   = query.value(3).toInt();
        data.artist     = query.value(4).toString();
        data.album      = query.value(5).toString();
        data.coverImage = query.value(6).toByteArray();
        data.genre      = query.value(7).toString();
        data.format     = query.value(8).toString();
    }

    return data;
}
