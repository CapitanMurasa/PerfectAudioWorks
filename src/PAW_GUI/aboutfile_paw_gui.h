#ifndef ABOUTFILE_PAW_GUI_H
#define ABOUTFILE_PAW_GUI_H

#include <QMainWindow>
#include <QTableWidgetItem> 
#include <QPixmap>
#include "../miscellaneous/DatabaseManager.h" 

namespace Ui {
    class Aboutfile_PAW_gui;
}

class Aboutfile_PAW_gui : public QMainWindow
{
    Q_OBJECT

public:
    explicit Aboutfile_PAW_gui(DatabaseManager* db, QWidget* parent = nullptr);
    ~Aboutfile_PAW_gui();

    void setdata(const QString& filename);
    void setdata(QTableWidgetItem* item); 

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::Aboutfile_PAW_gui* ui;
    QPixmap m_originalAlbumArt;
    DatabaseManager* m_database;

    void updateAlbumArt();
};

#endif 