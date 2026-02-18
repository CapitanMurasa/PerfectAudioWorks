#ifndef ABOUTFILE_PAW_GUI_H
#define ABOUTFILE_PAW_GUI_H

#include <QMainWindow>
#include <QPixmap>
#include <QListWidgetItem> 
#include <QMessageBox>  

QT_BEGIN_NAMESPACE
namespace Ui { class Aboutfile_PAW_gui; }
QT_END_NAMESPACE

class Aboutfile_PAW_gui : public QMainWindow
{
    Q_OBJECT

public:
    explicit Aboutfile_PAW_gui(QWidget* parent = nullptr);
    ~Aboutfile_PAW_gui() override;


    void setdata(QListWidgetItem* item);

    void setdata(const QString& filename);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::Aboutfile_PAW_gui* ui;
    QPixmap m_originalAlbumArt; 
    int Samplerate;
    int channels;
    int bitrate;
    void updateAlbumArt();
};
#endif