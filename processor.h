#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <memory>

#include "opencv2/highgui/highgui.hpp" //VideoCapture
#include "opencv2/calib3d/calib3d.hpp" //StereoSGBM
#include "arguments.hpp"

class Processor
{
public:
    Processor(Arguments& args, cv::VideoCapture& input_feed);

    std::shared_ptr<cv::VideoWriter> create_writer();

    std::shared_ptr<cv::Mat> process_frame(size_t frame_index);
    void process_frame(size_t frame_index, cv::VideoWriter& output_feed);
    void process_range(size_t start_frame, size_t end_frame, cv::VideoWriter& output_feed);
    void process_clip(cv::VideoWriter& output_feed);
private:
    Arguments& arguments;
    cv::VideoCapture& input;
    cv::StereoSGBM mapper;

    size_t input_width, input_height, split_width, output_width, output_height;
};

#endif // PROCESSOR_H
