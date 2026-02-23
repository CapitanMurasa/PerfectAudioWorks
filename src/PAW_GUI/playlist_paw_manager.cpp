#include "playlist_paw_manager.h"

Playlist_Paw_Manager::Playlist_Paw_Manager(DatabaseManager* db, Main_PAW_widget* parent = nullptr)
	: QDialog(parent)
	, ui(new Ui::PlaylistManager)
	, database(db)
{
	ui->setupUi(this);

}