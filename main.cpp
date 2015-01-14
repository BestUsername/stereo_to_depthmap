#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <stdio.h>
#include <argp.h>
#include <iostream>

const char* argp_program_version = "stereo_to_depthmap 1.0";
const char* argp_program_bug_address = "<bugs@marc.zone>";

bool to_bool(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	std::istringstream is(str);
	bool b;
	is >> std::boolalpha >> b;
	return b;
}

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	char *args[2];            /* infile and outfile */
	int verbose;              /* The -v flag */
	int output_fourcc;
	int num_disparities;
	int SAD_window_size;
	bool output_SBS;
	bool output_colour;
};

/*
OPTIONS.  Field 1 in ARGP.
Order of fields: {NAME, KEY, ARG, FLAGS, DOC}.
*/
static struct argp_option options[] =
{
	{"verbose", 'v', 0, 0, "Produce verbose output"},
	{"fourcc", 'f', "ABCD", 0, "Four lettercode for the output codec"},
	{"sbs",  's', 0, 0, "Output side-by-side video with depth map replacing left eye region"},
	{"colour", 'c', 0, 0, "Output video with colour channels"},
	{"disparity", 'd', "NUMBER", 0, "Number of pixels to search across. Needs to be divisible by 16"},
	{"window", 'w', "DIM", 0, "Dimension of window to compare against. Needs to be odd"},
	{0}
};

/*
PARSER. Field 2 in ARGP.
Order of parameters: KEY, ARG, STATE.
*/
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
	struct arguments *arguments = (struct arguments*)(state->input);

	switch (key)
	{
		case 'v':
			arguments->verbose = 1;
			break;
		case 'f':
			if (strlen(arg) == 4)
			{
				arguments->output_fourcc = CV_FOURCC(arg[0],arg[1],arg[2],arg[3]);
			}
			break;
		case 's':
			arguments->output_SBS = true;
			break;
		case 'c':
			arguments->output_colour = true;
			break;
		case 'd':
			arguments->num_disparities = atoi(arg);
			break;
		case 'w':
			arguments->SAD_window_size = atoi(arg);
			break;
		case ARGP_KEY_NO_ARGS:
			argp_usage(state);
			break;
		case ARGP_KEY_ARG:
			if (state->arg_num >= 2)
			{
				argp_usage(state);
			}
			arguments->args[state->arg_num] = arg;
			break;
		case ARGP_KEY_END:
			if (state->arg_num < 2)
			{
				argp_usage (state);
			}
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/*
ARGS_DOC. Field 3 in ARGP.
A description of the non-option command-line arguments
that we accept.
*/
static char args_doc[] = "INFILE OUTFILE";

/*
DOC.  Field 4 in ARGP.
Program documentation.
*/
static char doc[] = "stereo_to_depthmap -- A program to calculate a depth map video from a 3D Side-By-Side source video.";

/*
The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, args_doc, doc};

int main( int argc, char** argv )
{
	struct arguments arguments;
	arguments.output_fourcc = CV_FOURCC('D','I','V','X');;
	arguments.num_disparities = 16;
	arguments.SAD_window_size = 15;
	arguments.output_SBS = false;
	arguments.output_colour = false;
	
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	std::string input_filename = std::string(arguments.args[0]);
	std::string output_filename = std::string(arguments.args[1]);
	
	std::string windowTitle = "DepthMap";
	cv::namedWindow( windowTitle, cv::WINDOW_AUTOSIZE );

	cv::VideoCapture feed_src; //source video feed
	cv::VideoWriter  feed_dst; //destination video feed


	int num_disparities = arguments.num_disparities;
	int SAD_window_size = arguments.SAD_window_size;;
	
	double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps;
	bool input_colour;
	int  output_fourcc = arguments.output_fourcc;
	bool output_colour = arguments.output_colour;
	bool output_SBS = arguments.output_SBS;;

	{
		bool valid = true;
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

			feed_dst.open(output_filename, output_fourcc, output_fps, cv::Size(output_width, output_height), output_colour);
			if (!feed_dst.isOpened()) {
				std::cerr << "ERROR:\tOutput file [" << output_filename << "] cannot be opened for writing" << std::endl;
				valid = false;
			}
		}
		if (!valid) {
			return EXIT_FAILURE;
		}
	}
	
	std::cout << "Source Video:\t" << input_width << "x" << input_height << " @ " << input_fps << "fps" << std::endl;
	
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

