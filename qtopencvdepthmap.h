#ifndef QTOPENCVDEPTHMAP_H
#define QTOPENCVDEPTHMAP_H

#include <QMainWindow>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "arguments.hpp"

namespace Ui {
    class QtOpenCVDepthmap;
}

class QtOpenCVDepthmap : public QMainWindow
{
        Q_OBJECT

    public:
        explicit QtOpenCVDepthmap(Arguments& args, QWidget *parent = 0);
        ~QtOpenCVDepthmap();

        void set_active(bool isActive);

        void args_to_mapper();
        void open_filename(const std::string &filename);
        void fetch_frame(int index);
        void update_depthmap();

        template<typename Val>
        bool update_mapper_value(const Arguments::Arg &arg, const Val &val) {
            bool req_update = false;
            //get the current value
            Val previous = arguments.get_value<Val>(arg);
            //attempt to set the value
            arguments.set_value<Val>(arg, val);
            //test if value is valid. If not, attempt to rectify it
            if (!arguments.is_valid(arg, true)) {
                //wasn't able to correct the setting. Convert back to old value;
                arguments.set_value<Val>(arg, previous);
            }
            Val current = arguments.get_value<Val>(arg);
            //if our current value is different than the supplied value, then we need to update the gui controls
            if (val != current) {
                //this shouldn't happen as the inputs are already configured with ranges and step values
                req_update = true;
            }
            //only update depthmap if it changed.
            if (current != previous) {
                args_to_mapper();
                if (is_active) {
                    update_depthmap();
                }
            }
            return req_update;
        }

        template<typename T>
        void correct_range(T& value, T min, T max);

    public slots:

        void check_end_frame(int value);
        void check_start_frame(int value);
        void check_current_frame(int value);

    private slots:
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

        void on_horizontalSlider_valueChanged(int value);


        void on_actionQuit_triggered();

        void on_actionExport_triggered();

private:
        Arguments& arguments;

        Ui::QtOpenCVDepthmap *ui;
        bool is_active;

        cv::StereoSGBM mapper;

        //this chunk of variables handle video frame data
        cv::VideoCapture feed_src;
        cv::Mat frame_src, left_eye, right_eye, frame_dst_16_gray, frame_dst_8_gray, frame_dst_8_colour;

        //this chunk of variables handle video metadata
        double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps,
        current_pos_msec, current_pos_frame, current_pos_radio, input_frame_count;
};

#endif // QTOPENCVDEPTMAP_H
