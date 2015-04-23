#ifndef QSLIDERSUBRANGE_H
#define QSLIDERSUBRANGE_H

#include <QSlider>
#include <QColor>

/**
 * This adds the ability to display a double-ended sub-range on a QSlider.
 */
class QSliderSubRange : public QSlider
{
    Q_OBJECT
public:
    explicit QSliderSubRange(QWidget *parent = 0);

    virtual void paintEvent(QPaintEvent *event);

signals:

public slots:
    void sub_range_start_changed(int value);
    void sub_range_end_changed(int value);
protected:
    int sub_range_start_frame;
    int sub_range_end_frame;
    int sub_range_vert_thickness;
    QColor sub_range_colour;
};

#endif // QSLIDERSUBRANGE_H
