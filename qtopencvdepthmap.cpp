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
