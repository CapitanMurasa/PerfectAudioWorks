#include "loadingplaylists.h"
#include "main_paw_widget.h"
#include "ui_loadingplaylists.h" 


loadingplaylists::loadingplaylists(Main_PAW_widget* parent)
    : QDialog(parent),
    ui(new Ui::loadingplaylists),
    parentwidget(parent)
{
    ui->setupUi(this);
}

loadingplaylists::~loadingplaylists() {
    delete ui;
}

void loadingplaylists::inflateloadingbar(int value, QString label) {
    ui->progressbar->setValue(value);
    ui->DebugInfo->setText(label);
}