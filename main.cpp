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


	int num_disparities = 16;
	int SAD_window_size = 15;
	
	double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps;
	bool input_colour;
	int  output_fourcc;
	bool output_colour = true;
	bool output_SBS = true;

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
				input_height = feed_src.get(CV_CAP_PROP_FRAME_HEIGHT);
				input_fps = feed_src.get(CV_CAP_PROP_FPS);
				//int input_fourcc = feed_src.get(CV_CAP_PROP_FOURCC);
				split_width = input_width * 0.5;
				
				output_width = output_SBS ? input_width : split_width;
				output_height = input_height;
				output_fps = input_fps;
				//output_fourcc = input_fourcc;
				output_fourcc = CV_FOURCC('D','I','V','X');

				feed_dst.open(output_filename, output_fourcc, output_fps, cv::Size(output_width, output_height), output_colour);
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
	
	cv::Mat frame_src_colour, frame_src_gray, frame_dst, frame_dst_8bit_gray, frame_dst_8bit_colour, left_eye, right_eye;
	
	cv::StereoBM mapper(CV_STEREO_BM_BASIC, num_disparities, SAD_window_size);

	//init first frame from VideoCapture
	feed_src >> frame_src_colour;
	//detect if frame_src is colour (usally is)
	input_colour = frame_src_colour.channels() > 1;
	//loop while there's current video frame data and nothing has been pressed
	//step prep next frame
	for (; frame_src_colour.data && (cv::waitKey(33) < 0); feed_src >> frame_src_colour) {

		//input is assumed to be colour. If not, need to swap the frame_src_[gray/colour] to make sure that they are what they say they are
		if (input_colour) 
		{
			cv::cvtColor(frame_src_colour, frame_src_gray, CV_BGR2GRAY);
		} else {
			frame_src_gray = frame_src_colour;
			//if outputting colour and SBS, then will need a colour version of the source to overlay output on
			//kind of useless since this will only output a larger file.
			if (output_colour && output_SBS) {
				cv::cvtColor(frame_src_gray, frame_src_colour, CV_GRAY2BGR);
			}
		}

		left_eye = frame_src_gray.colRange(0, split_width);
		right_eye = frame_src_gray.colRange(split_width, input_width);

		mapper(left_eye, right_eye, frame_dst);
		
		//the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1 or CV_8UC3
		frame_dst.convertTo(frame_dst_8bit_gray, CV_8UC1);
		if (output_colour) cv::cvtColor(frame_dst_8bit_gray, frame_dst_8bit_colour, CV_GRAY2BGR);
		
		if (output_SBS) {
			if (output_colour) {
				frame_dst_8bit_colour.copyTo(frame_src_colour(cv::Rect(0,0,frame_dst_8bit_colour.cols, frame_dst_8bit_colour.rows)));
				feed_dst << frame_src_colour;
				cv::imshow( windowTitle, frame_src_colour );
			} else {
				frame_dst_8bit_gray.copyTo(frame_src_gray(cv::Rect(0,0,frame_dst_8bit_gray.cols, frame_dst_8bit_gray.rows)));
				feed_dst << frame_src_gray;
				cv::imshow( windowTitle, frame_src_gray );
			}
		} else {
			feed_dst << (output_colour ? frame_dst_8bit_colour : frame_dst_8bit_gray);
			cv::imshow( windowTitle, frame_dst_8bit_gray );
		}
	}

	return EXIT_SUCCESS;
}

