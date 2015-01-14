#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <stdio.h>
#include <iostream>

int main( int argc, char** argv )
{
	std::string windowTitle = "DepthMap";
	std::string input_filename, output_filename;
	cv::namedWindow( windowTitle, cv::WINDOW_AUTOSIZE );

	cv::VideoCapture feed_src; //source video feed
	cv::VideoWriter  feed_dst; //destination video feed

	bool outputSBS = true;

	int num_disparities = 16;
	int SAD_window_size = 15;
	
	double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps;
	int fourcc = CV_FOURCC('D','I','V','X');
	int output_type = CV_8UC3;
	bool output_colour = true;

	//verify inputs
	{
		bool valid = true;

		if (argc == 3) {
			input_filename = argv[1];
			output_filename = argv[2];

			feed_src.open(input_filename);
			if (!feed_src.isOpened()) {
				std::cerr << "ERROR:\tInput file [" << input_filename << "] cannot be opened for reading" << std::endl;
				valid = false;
			} else {
				input_width = feed_src.get(CV_CAP_PROP_FRAME_WIDTH);
				split_width = input_width * 0.5;
				input_height = feed_src.get(CV_CAP_PROP_FRAME_HEIGHT);
				input_fps = feed_src.get(CV_CAP_PROP_FPS);
				
				output_width = outputSBS ? input_width : split_width;
				output_height = input_height;
				output_fps = input_fps;
				
				feed_dst.open(output_filename, fourcc, output_fps, cv::Size(output_width, output_height), output_colour);
				if (!feed_dst.isOpened()) {
					std::cerr << "ERROR:\tOutput file [" << output_filename << "] cannot be opened for writing" << std::endl;
					valid = false;
				}
			}
		} else {
			std::cerr << "USAGE:\n\t" << argv[0] << " [Source Video] [Destination Video]" << std::endl;
			valid = false;
		}

		if (!valid) {
			return EXIT_FAILURE;
		}
	}
	
	std::cout << "Source Video:\t" << input_width << "x" << input_height << " @ " << input_fps << "fps" << std::endl;
	std::cout << "Dest Video:\t" << output_width << "x" << output_height << std::endl;
	
	cv::Mat frame_src, frame_src_post, frame_dst, frame_dst_post, frame_dst_post2, left_eye, right_eye;
	
	cv::StereoBM mapper(CV_STEREO_BM_BASIC, num_disparities, SAD_window_size);

	//init first frame from VideoCapture
	//loop while there's current video frame data and nothing has been pressed
	//step prep next frame
	for (feed_src >> frame_src; frame_src.data && (cv::waitKey(33) < 0); feed_src >> frame_src) {

		//convert colour input to greyscale for single channel disparity
		cv::cvtColor(frame_src, frame_src_post, CV_BGR2GRAY);

		left_eye = frame_src_post.colRange(0, split_width);
		right_eye = frame_src_post.colRange(split_width, input_width);

		mapper(left_eye, right_eye, frame_dst);
		
		//the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1 or CV_8UC3
		frame_dst.convertTo(frame_dst_post, CV_8UC1);
		if (output_colour) cv::cvtColor(frame_dst_post, frame_dst_post2, CV_GRAY2BGR);
		
		if (outputSBS) {
			
			if (output_colour) {
				frame_dst_post2.copyTo(frame_src(cv::Rect(0,0,frame_dst_post2.cols, frame_dst_post2.rows)));
			} else {
				frame_dst_post.copyTo(frame_src(cv::Rect(0,0,frame_dst_post.cols, frame_dst_post.rows)));
			}
			feed_dst << frame_src;
			cv::imshow( windowTitle, frame_src );
		} else {
			feed_dst << (output_colour ? frame_dst_post2 : frame_dst_post);
			cv::imshow( windowTitle, frame_dst_post );
		}
	}

	return EXIT_SUCCESS;
}

