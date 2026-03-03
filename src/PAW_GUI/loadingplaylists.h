#pragma once 

#include <QDialog>

class Main_PAW_widget;

namespace Ui {
    class loadingplaylists;
}

class loadingplaylists : public QDialog
{
    Q_OBJECT

public:
    explicit loadingplaylists(Main_PAW_widget* parent = nullptr);
    ~loadingplaylists();

    void inflateloadingbar(int value, QString label);

private:
    Ui::loadingplaylists* ui;
    Main_PAW_widget* parentwidget;
};