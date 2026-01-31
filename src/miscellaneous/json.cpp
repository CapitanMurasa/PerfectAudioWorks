#include "json.h"
#include <QDebug>

bool JsonLoader::load_jsonfile(json& j, QString filename) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    fileName = dir.filePath(filename);

    std::ifstream file(fileName.toStdString());
    if (!file.is_open()) {
        return false; 
    }

    try {
        file >> j;
        return true;
    }
    catch (json::parse_error& e) {
        return false;
    }
}

void JsonLoader::save_config(const json& j) {
    std::ofstream file(fileName.toStdString());
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
    else {
        qWarning() << "Could not open file for saving:" << fileName;
    }
}