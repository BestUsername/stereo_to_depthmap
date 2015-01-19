#include <QFileDialog>

#include "qtopencvdepthmap.h"
#include "ui_qtopencvdepthmap.h"

QtOpenCVDepthmap::QtOpenCVDepthmap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QtOpenCVDepthmap)
{
    ui->setupUi(this);
}

QtOpenCVDepthmap::~QtOpenCVDepthmap()
{
    delete ui;
}

void QtOpenCVDepthmap::on_actionStart_triggered()
{
    if( !mCapture.isOpened() )
        if( !mCapture.open( 0 ) )
            return;

    startTimer(0);
}

void QtOpenCVDepthmap::timerEvent(QTimerEvent *event)
{
    cv::Mat image;
    mCapture >> image;

    // Do what you want with the image :-)

    // Show the image
    ui->sbs_view->showImage( image );
}

void QtOpenCVDepthmap::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
         tr("Open Video"), "", tr("Video Files (*.avi *.mpg *.mp4);;All Files (*.*)"));
}

void QtOpenCVDepthmap::on_input_minDisparity_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_numDisparities_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_SADWindowSize_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_P1_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_P2_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_disp12MAxDiff_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_preFilterCap_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_uniqunessRatio_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_speckleWindowSize_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_speckleRange_valueChanged(int arg1)
{

}

void QtOpenCVDepthmap::on_input_fullDP_stateChanged(int arg1)
{
    /*
     Constant           Value   Description
     Qt::Unchecked          0   The item is unchecked.
     Qt::PartiallyChecked   1   The item is partially checked. Items in hierarchical models may be partially checked if some, but not all, of their children are checked.
     Qt::Checked            2   The item is checked.
    */
}
