#ifndef QTOPENCVDEPTHMAP_H
#define QTOPENCVDEPTHMAP_H

#include <QMainWindow>

#include <opencv2/highgui/highgui.hpp>

namespace Ui {
    class QtOpenCVDepthmap;
}

class QtOpenCVDepthmap : public QMainWindow
{
    Q_OBJECT

public:
    explicit QtOpenCVDepthmap(QWidget *parent = 0);
    ~QtOpenCVDepthmap();

private slots:
    void on_actionStart_triggered();

private:
    Ui::QtOpenCVDepthmap *ui;

    cv::VideoCapture mCapture;

protected:
    void timerEvent(QTimerEvent *event);
};

#endif // QTOPENCVDEPTMAP_H
