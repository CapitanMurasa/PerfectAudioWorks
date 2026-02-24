#include "ui_loadingplaylists.h"
#include "loadingplaylists.h"

void loadingplaylists(Main_PAW_widget parent) :
	parentwidget(parent)
	, ui(new Ui::About_PAW_gui) 
{
	ui->Show(this);
}
void ~loadingplaylists() {
	delete ui;
}

void inflateloadingbar(int value, QString label) {

	ui->progressbar->setValue(value);
	ui->label->setText(label);
}