#include <iostream> //TODO: remove this

#include "opencv2/imgproc/imgproc.hpp" //CV_Gray2RGB cvtColor
#include "opencv2/calib3d/calib3d.hpp" //StereoSGBM
#include "opencv2/highgui/highgui.hpp" //CV_FOURCC, VideoCapture

#include "processor.h"

Processor::Processor(Arguments& args, cv::VideoCapture& input_feed)
    : arguments(args), input(input_feed)
{
    input_width   = (size_t)input.get(CV_CAP_PROP_FRAME_WIDTH);
    input_height  = (size_t)input.get(CV_CAP_PROP_FRAME_HEIGHT);
    split_width   = input_width / 2;
    output_width  = split_width;
    output_height = input_height;
}

std::shared_ptr<cv::VideoWriter> Processor::create_writer() {
    //get the output filename
    std::string output_filename = arguments.get_value<std::string>(Arguments::OUTPUT_FILENAME);
    int output_fourcc           = arguments.get_value<int>(Arguments::OUTPUT_FOURCC);

    double fps = input.get(CV_CAP_PROP_FPS);

    std::shared_ptr<cv::VideoWriter> output_feed(new cv::VideoWriter());
    output_feed->open(output_filename, output_fourcc, fps, cv::Size(output_width, output_height), true);

    return output_feed;
}

//frame_index is 0-indexed
std::shared_ptr<cv::Mat> Processor::process_frame(size_t frame_index) {
    //Update mapper arguments

    mapper.minDisparity        = arguments.get_value<int>  (Arguments::MIN_DISPARITY);
    mapper.numberOfDisparities = arguments.get_value<int>  (Arguments::NUM_DISPARITIES);
    mapper.SADWindowSize       = arguments.get_value<int>  (Arguments::SAD_WINDOW_SIZE);
    mapper.P1                  = arguments.get_value<int>  (Arguments::P1);
    mapper.P2                  = arguments.get_value<int>  (Arguments::P2);
    mapper.disp12MaxDiff       = arguments.get_value<int>  (Arguments::DISP12_MAX_DIFF);
    mapper.preFilterCap        = arguments.get_value<int>  (Arguments::PRE_FILTER_CAP);
    mapper.uniquenessRatio     = arguments.get_value<int>  (Arguments::UNIQUENESS);
    mapper.speckleWindowSize   = arguments.get_value<int>  (Arguments::SPECKLE_WINDOW_SIZE);
    mapper.speckleRange        = arguments.get_value<int>  (Arguments::SPECKLE_RANGE);
    mapper.fullDP              = arguments.get_value<bool> (Arguments::FULL_DP);

    cv::Mat frame_src, left_eye, right_eye, frame_dst_16_gray, frame_dst_8_gray;
    std::shared_ptr<cv::Mat> output_frame(new cv::Mat());

    //set our specified starting frame to be the next captured
    input.set(CV_CAP_PROP_POS_FRAMES, frame_index);
    //capture current frame to matrix
    input >> frame_src;

    //split the source frame into left and right eye frames
    left_eye = frame_src.colRange(0, split_width);
    right_eye = frame_src.colRange(split_width, input_width);

    //use mapper settings to preform a disparity calculation
    mapper(left_eye, right_eye, frame_dst_16_gray);

    //the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1
    frame_dst_16_gray.convertTo(frame_dst_8_gray, CV_8UC1);
    cvtColor(frame_dst_8_gray, *output_frame, CV_GRAY2RGB);

    return output_frame;
}

void Processor::process_frame(size_t frame_index, cv::VideoWriter& output_feed) {
    output_feed << *(process_frame(frame_index));
}

//frame numbers are 0-indexed
void Processor::process_range(size_t start_frame, size_t end_frame, cv::VideoWriter& output_feed) {
    for (size_t index = start_frame; index <= end_frame; ++index) {
        process_frame(index, output_feed);
    }

}

void Processor::process_clip(cv::VideoWriter& output_feed) {
    size_t start_frame = arguments.get_value<int>(Arguments::START_FRAME);
    size_t end_frame   = arguments.get_value<int>(Arguments::END_FRAME);
    process_range(start_frame, end_frame, output_feed);
}
