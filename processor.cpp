#include <iostream> //TODO: remove this

#include "opencv2/imgproc/imgproc.hpp" //CV_Gray2RGB cvtColor
#include "opencv2/calib3d/calib3d.hpp" //StereoSGBM
#include "opencv2/highgui/highgui.hpp" //CV_FOURCC, VideoCapture

#include "processor.h"

void Processor::process_clip(cv::VideoCapture& feed_src, Arguments& arguments) {
    cv::VideoWriter  feed_dst; //destination video feed
    double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps, total_frames;
    //open iostreams and set dimension variables

    bool valid = true;

    std::string input_filename = arguments.get_value<std::string>(Arguments::INPUT_FILENAME);
    std::string output_filename = arguments.get_value<std::string>(Arguments::OUTPUT_FILENAME);
    int output_fourcc = arguments.get_value<int>(Arguments::OUTPUT_FOURCC);
    //if the VideoCapture file hasn't already been opened, might as well try
    if (!feed_src.isOpened()) {
        feed_src.open(input_filename);
    }
    if (!feed_src.isOpened()) {
        std::cerr << "ERROR:\tInput file [" << input_filename << "] cannot be opened for reading" << std::endl;
        valid = false;
    } else {
        total_frames = feed_src.get(CV_CAP_PROP_FRAME_COUNT);
        input_width = feed_src.get(CV_CAP_PROP_FRAME_WIDTH);
        input_height = feed_src.get(CV_CAP_PROP_FRAME_HEIGHT);
        input_fps = feed_src.get(CV_CAP_PROP_FPS);
        split_width = input_width * 0.5;

        output_width = split_width;
        output_height = input_height;
        output_fps = input_fps;

        feed_dst.open(output_filename, output_fourcc, output_fps, cv::Size(output_width, output_height), true);
        if (!feed_dst.isOpened()) {
            std::cerr << "ERROR:\tOutput file [" << output_filename << "] cannot be opened for writing" << std::endl;
            valid = false;
        }
    }
    if (!valid) {
        std::cerr << "ERROR:\tINVALID" << std::endl;
    } else {

        cv::Mat frame_src, left_eye, right_eye, frame_dst_16_gray, frame_dst_8_gray, frame_dst_8_colour;

        int start_frame         = arguments.get_value<int>  (Arguments::START_FRAME);
        int end_frame           = arguments.get_value<int>  (Arguments::END_FRAME);
        int min_disparity       = arguments.get_value<int>  (Arguments::MIN_DISPARITY);
        int num_disparities     = arguments.get_value<int>  (Arguments::NUM_DISPARITIES);
        int SAD_window_size     = arguments.get_value<int>  (Arguments::SAD_WINDOW_SIZE);
        int p1                  = arguments.get_value<int>  (Arguments::P1);
        int p2                  = arguments.get_value<int>  (Arguments::P2);
        int disp12_max_diff     = arguments.get_value<int>  (Arguments::DISP12_MAX_DIFF);
        int pre_filter_cap      = arguments.get_value<int>  (Arguments::PRE_FILTER_CAP);
        int uniqueness          = arguments.get_value<int>  (Arguments::UNIQUENESS);
        int speckle_window_size = arguments.get_value<int>  (Arguments::SPECKLE_WINDOW_SIZE);
        int speckle_range       = arguments.get_value<int>  (Arguments::SPECKLE_RANGE);
        bool full_dp            = arguments.get_value<bool> (Arguments::FULL_DP);
        cv::StereoSGBM mapper(min_disparity, num_disparities, SAD_window_size,
                              p1, p2, disp12_max_diff, pre_filter_cap,
                              uniqueness, speckle_window_size, speckle_range,
                              full_dp);

        //remove default 0 for start_frame to mean first - needs to be 1-indexed
        start_frame = std::max(1, start_frame);
        //change default 0 for end_frame to mean last - needs to be 1-indexed
        if (0 == end_frame) {
            end_frame = total_frames;
        } else {
            //end_frame has been set.
            //on the off chance a bad end_frame value came in, cap it between the start_frame and total number of frames
            end_frame = std::min(std::max(end_frame, start_frame), (int)total_frames);
        }
        //set our specified starting frame to be the next captured
        feed_src.set(CV_CAP_PROP_POS_FRAMES, start_frame);

        //for the length of the selected clip (by default full video)
        for (int i = start_frame; i <= end_frame; ++i) {
            feed_src >> frame_src;

            //split the source frame into left and right eye frames
            left_eye = frame_src.colRange(0, split_width);
            right_eye = frame_src.colRange(split_width, input_width);

            //use mapper settings to preform a disparity calculation
            mapper(left_eye, right_eye, frame_dst_16_gray);

            //the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1
            frame_dst_16_gray.convertTo(frame_dst_8_gray, CV_8UC1);
            cvtColor(frame_dst_8_gray, frame_dst_8_colour, CV_GRAY2RGB);
            //write to our destination feed
            feed_dst << frame_dst_8_colour;
        }
    }
}

