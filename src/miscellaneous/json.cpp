#include "json.h"
#include <QDebug>
#include <algorithm>

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

void JsonLoader::save_config(const json& j, QString filename) {
    std::ofstream file(fileName.toStdString());
    if (file.is_open()) {
        file << j.dump(4);
        file.close();
    }
    else {
        qWarning() << "Could not open file for saving:" << fileName;
    }
}

int JsonLoader::FindItemInArray(json& j, std::string searchPath) {
    auto it = std::find(j.begin(), j.end(), searchPath);

    if (it != j.end()) {
        return static_cast<int>(std::distance(j.begin(), it));
    }
    else {
        return -1;
    }
}

void JsonLoader::RemoveItemByIndex(json& j, int index) {
    if (!j.is_array()) return;

    if (index >= 0 && index < static_cast<int>(j.size())) {
        j.erase(j.begin() + index);
    }
}