#ifndef SCROLLINGLABEL_H
#define SCROLLINGLABEL_H

#include <QLabel>
#include <QTimer>
#include <QPainter>

class ScrollingLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ScrollingLabel(QWidget* parent = nullptr);

    void setText(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onTimerTimeout();

private:
    QTimer* timer;
    int offset;        
    int textWidth;      
    bool isScrolling;  
    const int speed = 2;       
    const int pauseTime = 2000; 
};

#endif 