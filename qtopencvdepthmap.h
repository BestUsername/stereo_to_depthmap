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
    void on_actionOpen_triggered();

    void on_input_minDisparity_valueChanged(int arg1);

    void on_input_numDisparities_valueChanged(int arg1);

    void on_input_SADWindowSize_valueChanged(int arg1);

    void on_input_P1_valueChanged(int arg1);

    void on_input_P2_valueChanged(int arg1);

    void on_input_disp12MAxDiff_valueChanged(int arg1);

    void on_input_preFilterCap_valueChanged(int arg1);

    void on_input_uniqunessRatio_valueChanged(int arg1);

    void on_input_speckleWindowSize_valueChanged(int arg1);

    void on_input_speckleRange_valueChanged(int arg1);

    void on_input_fullDP_stateChanged(int arg1);

private:
    Ui::QtOpenCVDepthmap *ui;

    cv::VideoCapture mCapture;

protected:
    void timerEvent(QTimerEvent *event);
};

#endif // QTOPENCVDEPTMAP_H
