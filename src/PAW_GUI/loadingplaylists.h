#include "main_paw_widget.h"
class Main_PAW_widget;


class loadingplaylists : public QDialog
{
    Q_OBJECT

public:
    loadingplaylists(Main_PAW_widget parent = nullptr);
    ~loadingplaylists();

private:
    Ui::Dialog* ui;
    Main_PAW_widget* parentwidget;

    void inflateloadingbar(int value, QString label);

};