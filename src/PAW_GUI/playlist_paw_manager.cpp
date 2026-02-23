#include "playlist_paw_manager.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>

Playlist_Paw_Manager::Playlist_Paw_Manager(DatabaseManager* db, Main_PAW_widget* parent)
	: QDialog(parent)
	, ui(new Ui::PlaylistManager)
	, database(db)
{
	ui->setupUi(this);

	connect(ui->AddPlaylist, &QPushButton::clicked, this, &Playlist_Paw_Manager::Add_Playlist);
}

Playlist_Paw_Manager::~Playlist_Paw_Manager() {
	delete ui;
}

void Playlist_Paw_Manager::Add_Playlist() {
	bool ok;
	QString text = QInputDialog::getText(this, tr("Add playlist"),
		tr("Type the name of the playlist:"), QLineEdit::Normal,
		"your playlist name here...", &ok);

	if (ok && !text.isEmpty()) {
	
	}
}