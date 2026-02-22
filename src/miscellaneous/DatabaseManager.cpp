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

void DatabaseManager::FillRow(QString path) {
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return;
    }

    FileInfo info = { 0 };
    int metadata_result = -1;

#ifdef _WIN32
    std::wstring w_filePath = path.toStdWString();
    metadata_result = get_metadata_w(w_filePath.c_str(), &info);
#else
    QByteArray utf8_filePath = path.toUtf8();
    metadata_result = get_metadata(utf8_filePath.constData(), &info);
#endif

    QString title = (metadata_result == 0 && info.title && strlen(info.title) > 0)
        ? QString::fromUtf8(info.title) : path.section('/', -1);

    QString artist = (metadata_result == 0 && info.artist && strlen(info.artist) > 0)
        ? QString::fromUtf8(info.artist) : "Unknown Artist";

    QString album = (metadata_result == 0 && info.album && strlen(info.album) > 0)
        ? QString::fromUtf8(info.album) : "Unknown Album";

    QString genre = (metadata_result == 0 && info.genre && strlen(info.genre) > 0)
        ? QString::fromUtf8(info.genre) : "Unknown Genre";

    QByteArray coverBlob;
    if (metadata_result == 0 && info.cover_image && info.cover_size > 0) {
        coverBlob = QByteArray(reinterpret_cast<const char*>(info.cover_image), info.cover_size);
    }

    QSqlQuery query(db);

    query.prepare("INSERT OR IGNORE INTO genres (name) VALUES (:name)");
    query.bindValue(":name", genre);
    query.exec();

    query.prepare("SELECT id FROM genres WHERE name = :name");
    query.bindValue(":name", genre);
    query.exec();
    int genreId = -1;
    if (query.next()) genreId = query.value(0).toInt();

    query.prepare("INSERT OR IGNORE INTO artists (name) VALUES (:name)");
    query.bindValue(":name", artist);
    query.exec();

    query.prepare("SELECT id FROM artists WHERE name = :name");
    query.bindValue(":name", artist);
    query.exec();
    int artistId = -1;
    if (query.next()) artistId = query.value(0).toInt();

    query.prepare("INSERT OR IGNORE INTO albums (title, cover_image) VALUES (:title, :cover)");
    query.bindValue(":title", album);
    query.bindValue(":genre", genre); 

    if (coverBlob.isEmpty()) {
        query.bindValue(":cover", QVariant(QMetaType::fromType<QByteArray>()));
    }
    else {
        query.bindValue(":cover", coverBlob);
    }
    query.exec();

    query.prepare("SELECT id FROM albums WHERE title = :title");
    query.bindValue(":title", album);
    query.exec();
    int albumId = -1;
    if (query.next()) albumId = query.value(0).toInt();
    qDebug() << albumId;

    
    query.prepare("INSERT OR REPLACE INTO tracks (path, title, bitrate, genre_id, artist_id, album_id) "
        "VALUES (:path, :title, :bitrate, :genre_id, :artist_id, :album_id)");
    query.bindValue(":path", path);
    query.bindValue(":title", title);
    query.bindValue(":genre_id", genreId);
    query.bindValue(":bitrate", (metadata_result == 0) ? info.bitrate : 0);
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

    FileInfo_cleanup(&info);
}

void DatabaseManager::InflatePlaylist(QString path) {

}
TrackData DatabaseManager::LoadRow(QString path) {
    TrackData data;
    QSqlDatabase db = QSqlDatabase::database("PAW_CONNECTION");
    if (!db.transaction()) {
        qCritical() << "Failed to start database transaction:" << db.lastError().text();
        return data;
    }

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

    if (!db.commit()) {
        qCritical() << "Failed to commit track data to disk:" << db.lastError().text();
        db.rollback();
    }
    else {
        qDebug() << "Track inserted/updated successfully!";
    }

    return data;
}