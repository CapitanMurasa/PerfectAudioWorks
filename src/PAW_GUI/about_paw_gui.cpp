#include "about_paw_gui.h"
#include "ui_about_paw_gui.h"
#include <QLabel>

About_PAW_gui::About_PAW_gui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::About_PAW_gui)
{
    ui->setupUi(this);

}

About_PAW_gui::~About_PAW_gui()
{
    delete ui;
}
