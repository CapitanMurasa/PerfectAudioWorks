#pragma once

#include "main_paw_widget.h"
#include "../miscellaneous/DatabaseManager.h"
#include "ui_playlist_paw_manager.h"
#include "../miscellaneous/DatabaseManager.h"
#include <QDialog>

class Main_PAW_widget;
class DatabaseManager;

class Playlist_Paw_Manager : public QDialog 
{
    Q_OBJECT

public:
    explicit Playlist_Paw_Manager(DatabaseManager *db, Main_PAW_widget* parent = nullptr);
    ~Playlist_Paw_Manager();

private:
    Ui::PlaylistManager* ui;
    DatabaseManager *database;

    void Add_Playlist();
    void FetchPlaylists();
};