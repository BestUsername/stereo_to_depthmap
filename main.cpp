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
	arguments()
	{
		verbose = false;
		nogui = false;
		output_fourcc = CV_FOURCC_DEFAULT;
		input_filename = NULL;
		output_filename = strdup("output.avi");
		num_disparities = 16;
		SAD_window_size = 15;
		min_disparity = 0;
		pre_filter_cap = 0;
		uniqueness = 0;
		p1 = 0;
		p2 = 0;
		disp12_max_diff = 0;
		speckle_window_size = 0;
		speckle_range = 0;
		full_dp = false;
	}

	virtual ~arguments() {
		if (input_filename) {
			delete input_filename;
			input_filename = NULL;
		}
		if (output_filename) {
			delete output_filename;
			output_filename = NULL;
		}
	}

	bool verbose;
	bool nogui;
	int output_fourcc;
	char* input_filename;
	char* output_filename;
	int num_disparities;
	int SAD_window_size;
	int min_disparity;
	int pre_filter_cap;
	int uniqueness;
	int p1;
	int p2;
	int disp12_max_diff;
	int speckle_window_size;
	int speckle_range;
	bool full_dp;
};
/* set defaults for arguments structure before parsing */
/* 
   validate arguments structure after parsing.
   more of a logic-check as format validation is already performed in parse_opt
 */

/*
OPTIONS.  Field 1 in ARGP.
Order of fields: {NAME, KEY, ARG, FLAGS, DOC, GROUP}.
*/
static struct argp_option options[] =
{
	// TODO: check for parameter collision with Frameworks (Qt)
	{"verbose"          ,    'v',         0, 0,                                                                "Produce verbose output. Default false.", 0},
	{"nogui"            ,    'c',         0, 0,                                        "I, for one, welcome our command-line overlords! Default false.", 0},
	{"fourcc"           ,    'f',    "CODE", 0,                                                   "Four lettercode for the output codec. Default IYUV.", 1},
	{"infile"           ,    'i',  "INFILE", 0,                               "The video file to read from. Currently required for headless operation.", 1},
	{"outfile"          ,    'o', "OUTFILE", 0,                                                   "The video file to write out to. Default output.avi.", 1},
	{"disparity"        ,    'd',   "VALUE", 0,                           "Number of pixels to search across. Needs to be divisible by 16. Default 16.", 2},
	{"window"           ,    'w',   "VALUE", 0,                                  "Dimension of window to compare against. Needs to be odd. Default 15.", 2},
	{"minDisparity"     ,    'm',   "VALUE", 0,                                                               "Minimum disparity allowable. Default 0.", 3},
	{"truncate"         ,    't',   "VALUE", 0,                                       "Truncate pre-filter image pixel values to +/- VALUE. Default 0.", 3},
	{"uniqueness"       ,    'u',   "VALUE", 0,                                       "Truncate pre-filter image pixel values to +/- VALUE. Default 0.", 3},
	{"P1"               ,   1000,   "VALUE", 0,                          "First disparity smoothness value. Must be smaller than P2 if used Default 0.", 3},
	{"P2"               ,   1001,   "VALUE", 0,                         "Second disparity smoothness value. Must be larger than P1 if used. Default 0.", 3},
	{"maxDiff"          ,   1002,   "VALUE", 0,                               "Maximum allowable difference in left-right disparity check. Default -1.", 3},
	{"speckleWindowSize",   1003,   "VALUE", 0,                        "Maximum size of smooth disparity regions. Should be 50<=VALUE,=200. Default 0.", 3},
	{"speckleRange"     ,   1004,   "VALUE", 0,                       "Maximum disparity variation within each component. Should be 1 or 2. Default 0.", 3},
	{"fullDP"           ,   1005,         0, 0,    "If run the full-scale two-pass dynamic programming algorithm. Takes lots of memory. Default false.", 3},
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
		//group 0 - ungrouped
		case 'v': //verbose
			arguments->verbose = true;
			break;
		case 'c': //nogui
			arguments->nogui = true;
			break;
		//group 1 - input/output
		case 'f': //fourcc
			//enclosing variable initialization to prevent variable collision
			{
				//a fourcc code is valid iff made from 4 alphanumeric characters without punctuation
				//http://www.fourcc.org/codecs.php
				bool is_valid = false;
				int fourcc = 0;
				if (strlen(arg) == 4)
				{
					//check if [a-z,A-Z,0-9]
					if (isalnum(arg[0]) && isalnum(arg[1]) && isalnum(arg[2]) && isalnum(arg[3]))
					{
						fourcc = CV_FOURCC(arg[0],arg[1],arg[2],arg[3]);
						is_valid = true;
					}
				}
				if (is_valid) 
				{
					//won't know if the fourcc code is actually valid until initialization
					arguments->output_fourcc = fourcc;
				} else {
					argp_usage(state);
				}
			}
			break;
		case 'i': //infile
			if (arguments->input_filename) delete arguments->input_filename;
			arguments->input_filename = strdup(arg);
			break;
		case 'o': //outfile
			if (arguments->output_filename) delete arguments->output_filename;
			arguments->output_filename = strdup(arg);
			break;
		//group 2 - information shared between StereoSGBM and StereoBM
		case 'd': //disparity
			arguments->num_disparities = atoi(arg);
			break;
		case 'w': //SAD_window_size
			arguments->SAD_window_size = atoi(arg);
			break;
		//group 3 - information specific to StereoSGBM
		case 'm': //min_disparity
			arguments->min_disparity = atoi(arg);
			break;
		case 't': //truncate
			arguments->pre_filter_cap = atoi(arg);
			break;
		case 'u': //uniqueness
			arguments->uniqueness = atoi(arg);
			break;
		case 1000: //P1
			arguments->p1 = atoi(arg);
			break;
		case 1001: //P2
			arguments->p2 = atoi(arg);
			break;
		case 1002: //maxDiff
			arguments->disp12_max_diff = atoi(arg);
			break;
		case 1003: //speckleWindowSize
			arguments->speckle_window_size = atoi(arg);
			break;
		case 1004: //speckleRange
			arguments->speckle_range = atoi(arg);
			break;
		case 1005: //fullDP
			arguments->full_dp = true;
			break;
		case ARGP_KEY_NO_ARGS:
			//no arguments is valid
			break;
		case ARGP_KEY_ARG:
			//no current required input values. Should pass through to Framework libs like other unknown parameters.
		default:
			//With big frameworks (Qt), there could be additional parameters that we need to pass through
			//arguments->args[state->arg_num] = arg;
			break;
	}
	//argp_usage(state);
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
	arguments.nogui = false;
	arguments.output_fourcc = CV_FOURCC_DEFAULT; //linux-only way to allow auto detection based on filename
	arguments.num_disparities = 16;
	arguments.SAD_window_size = 15;
	
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	std::string windowTitle = "DepthMap";
	cv::namedWindow( windowTitle, cv::WINDOW_AUTOSIZE );

	cv::VideoCapture feed_src; //source video feed
	cv::VideoWriter  feed_dst; //destination video feed


	int num_disparities = arguments.num_disparities;
	int SAD_window_size = arguments.SAD_window_size;;
	
	double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps;
	bool input_colour;
	int  output_fourcc = arguments.output_fourcc;

	std::string input_filename = arguments.input_filename;
	std::string output_filename = arguments.output_filename;

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
			split_width = input_width * 0.5;
			
			output_width = split_width;
			output_height = input_height;
			output_fps = input_fps;

			feed_dst.open(output_filename, output_fourcc, output_fps, cv::Size(output_width, output_height), true /*[output_colour removed]*/); 
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
			//if outputting colour and [sbs REMOVED], then will need a colour version of the source to overlay output on
			//kind of useless since this will only output a larger file.
			if (true /* [output_colour removed] */) {
				cv::cvtColor(frame_src_gray, frame_src_colour, CV_GRAY2BGR);
			}
		}

		left_eye = frame_src_gray.colRange(0, split_width);
		right_eye = frame_src_gray.colRange(split_width, input_width);

		mapper(left_eye, right_eye, frame_dst);
		
		//the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1 or CV_8UC3
		frame_dst.convertTo(frame_dst_8bit_gray, CV_8UC1);
		if (true /*[output_colour removed]*/) cv::cvtColor(frame_dst_8bit_gray, frame_dst_8bit_colour, CV_GRAY2BGR);
		
		if (false /* [output_sbs removed] */) {
			if (true /* [output_colour removed] */) {
				frame_dst_8bit_colour.copyTo(frame_src_colour(cv::Rect(0,0,frame_dst_8bit_colour.cols, frame_dst_8bit_colour.rows)));
				feed_dst << frame_src_colour;
				cv::imshow( windowTitle, frame_src_colour );
			} else {
				frame_dst_8bit_gray.copyTo(frame_src_gray(cv::Rect(0,0,frame_dst_8bit_gray.cols, frame_dst_8bit_gray.rows)));
				feed_dst << frame_src_gray;
				cv::imshow( windowTitle, frame_src_gray );
			}
		} else {
			feed_dst << frame_dst_8bit_colour; /*(output_colour ? frame_dst_8bit_colour : frame_dst_8bit_gray); [output_colour removed]*/
			cv::imshow( windowTitle, frame_dst_8bit_gray );
		}
	}

	return EXIT_SUCCESS;
}

