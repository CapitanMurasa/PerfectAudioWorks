#include "ScrollingLabel.h"
#include <QFontMetrics>

ScrollingLabel::ScrollingLabel(QWidget* parent)
    : QLabel(parent), offset(0), isScrolling(false)
{
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ScrollingLabel::onTimerTimeout);
    timer->setSingleShot(false);
}

void ScrollingLabel::setText(const QString& text) {
    QLabel::setText(text);
    offset = 0;

    QFontMetrics fm(font());
    textWidth = fm.horizontalAdvance(text);

    if (textWidth > width()) {
        isScrolling = true;
        timer->start(30); 
    }
    else {
        isScrolling = false;
        timer->stop();
        update(); 
    }
}

void ScrollingLabel::resizeEvent(QResizeEvent* event) {
    QLabel::resizeEvent(event);
    setText(text());
}

void ScrollingLabel::paintEvent(QPaintEvent* event) {
    if (!isScrolling) {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);

    QRect drawRect = rect();


    int x = -offset;

    int y = (height() - fontMetrics().height()) / 2;

    painter.drawText(x, 0, textWidth + 50, height(), Qt::AlignVCenter | Qt::AlignLeft, text());
}

void ScrollingLabel::onTimerTimeout() {
    if (!isScrolling) return;

    offset += speed;


    if (offset > textWidth - width() + 50) {
        timer->stop();
        QTimer::singleShot(pauseTime, this, [this]() {
            offset = 0;
            if (isVisible()) timer->start(30);
            });
    }

    update(); 
}