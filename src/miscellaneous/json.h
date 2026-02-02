#ifndef JSON_H
#define JSON_H

#include <nlohmann/json.hpp>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <fstream>

using json = nlohmann::json;

class JsonLoader {
public:
    bool load_jsonfile(json& j, QString filename);

    void save_config(const json& j, QString filename);

    int FindItemInArray(json& j, std::string searchPath);

    void RemoveItemByIndex(json& j, int index);

private:
    QString fileName; 
};

#endif