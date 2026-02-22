#include "about_paw_gui.h"
#include "ui_about_paw_gui.h"
#include <QLabel>

About_PAW_gui::About_PAW_gui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::About_PAW_gui)
{
    ui->setupUi(this);

    QPixmap pixmap(":/assets/paw.ico");
    ui->logoLabel->setPixmap(pixmap);

}

About_PAW_gui::~About_PAW_gui()
{
    delete ui;
}
