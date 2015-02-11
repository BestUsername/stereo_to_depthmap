#include <QStyleOptionSlider>
#include <QPainter>

#include "qslidersubrange.h"

QSliderSubRange::QSliderSubRange(QWidget *parent) :
    QSlider(parent)
{
    sub_range_start_frame = 0;
    sub_range_end_frame = 100;
    sub_range_vert_thickness = 5;
    sub_range_colour = Qt::blue;
}

void QSliderSubRange::paintEvent(QPaintEvent *event) {
    //guarantee we don't div/0
    double temp_max = std::max(this->maximum(), 1);
    double sub_range_start_ratio = sub_range_start_frame / temp_max;
    double sub_range_width_ratio = (sub_range_end_frame - sub_range_start_frame) / temp_max;

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    opt.subControls = QStyle::SC_SliderGroove | QStyle::SC_SliderHandle;
    if (tickPosition() != NoTicks) {
        opt.subControls |= QStyle::SC_SliderTickmarks;
    }

    QRect groove_rect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    //qDebug() << groove_rect;
    QRect rect(groove_rect.left() + sub_range_start_ratio * groove_rect.width(), groove_rect.top() - sub_range_vert_thickness, sub_range_width_ratio * groove_rect.width(), groove_rect.height() + 2*sub_range_vert_thickness);
    QPainter painter(this);
    painter.fillRect(rect, QBrush(sub_range_colour));
    QSlider::paintEvent(event);

}

void QSliderSubRange::sub_range_start_changed(int value) {
    sub_range_start_frame = value;
    this->update();
}

void QSliderSubRange::sub_range_end_changed(int value) {
    sub_range_end_frame = value;
    this->update();
}
