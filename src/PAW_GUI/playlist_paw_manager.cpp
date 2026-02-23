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

	parentwidget = parent;

	if (database) {
		FetchPlaylists();
	}

	connect(ui->AddPlaylist, &QPushButton::clicked, this, &Playlist_Paw_Manager::Add_Playlist);
}

Playlist_Paw_Manager::~Playlist_Paw_Manager() {
	delete ui;
}

void Playlist_Paw_Manager::FetchPlaylists() {
    ui->listWidget->clear();

    QList<Playlistdata> allPlaylists = database->FetchPlaylists();

    for (const Playlistdata& p : allPlaylists) {
        QListWidgetItem *item = new QListWidgetItem(p.Name, ui->listWidget);

        item->setData(Qt::UserRole, p.id);
    }
}

void Playlist_Paw_Manager::LoadPlaylist() {
    QListWidgetItem* item = ui->listWidget->currentItem();

    if (item) {
        int id = item->data(Qt::UserRole).toInt();
		parentwidget->addFilesToPlaylistfromDatabase(id);
    }
}

void Playlist_Paw_Manager::Add_Playlist() {
	bool ok;
	QString text = QInputDialog::getText(this, tr("Add playlist"),
		tr("Type the name of the playlist:"), QLineEdit::Normal,
		"your playlist name here...", &ok);

	if (ok && !text.isEmpty()) {
		database->AddPlaylist(text.trimmed());
	}
	FetchPlaylists();

}