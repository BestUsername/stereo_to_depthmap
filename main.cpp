#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include <stdio.h>
#include <argp.h>
#include <iostream>

const char* argp_program_version = "stereo_to_depthmap 0.1";
const char* argp_program_bug_address = "<bugs@marc.zone>";

/* This structure is used by main to communicate with parse_opt. */
struct arguments
{
	arguments() {
		reset();
	}

	void reset() {
		verbose = false;
		nogui = false;
		output_fourcc = CV_FOURCC_DEFAULT;
		//next two must only point to dynamically allocated memory.
		input_filename = NULL;
		output_filename = NULL;
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

	bool set_fourcc(const char* code) {
		bool retval = false;

		//lambda to check if a specific character is valid for fourcc input
		auto is_fourcc_char = [](char character) {
			//there are currently 3 fourcc codes that include the space character
			// ("RLE ", "Y16 ", "Y8  ")
			return isalnum(character) || isspace(character);
		};

		if (strlen(code) == 4) {
			if (is_fourcc_char(code[0]) && is_fourcc_char(code[1]) && is_fourcc_char(code[2]) && is_fourcc_char(code[3])) {
				output_fourcc = CV_FOURCC(code[0], code[1], code[2], code[3]);
				retval = true;
			}
		}
		return retval;
	}

	bool is_valid(bool correct = false) {
		/*
		 rules:
		 *) verbose and full_dp are boolean and independent - so no validation
		 *) if nogui is set, filenames have to be set because pipes aren't handled yet.
	     *) if nogui is not set, filenames are optional
		 *) num_disparities has to be a multiple of 16 and >=0
		 *) min_disparity >= 0
		 *) SAD_window_size must be odd and >=1
		 *) P1/P2 have to both be >=0 and if they're not both 0, P2 > P1
		 *) disp12_max_diff *can* be anything, but *should* be >= -1
		 *) pre_filter_cap should be >=0
		 *) uniquness should be >=0
		 *) speckle_window_size >=0
		 *) speckle_range >=0
		*/
		
		bool valid = true;
		
		if (nogui) {
			if (!input_filename) {
				//can't correct this without using stdin/stdout
				valid = false;
			}
			if (!output_filename) {
				if (correct) {
					//this can be corrected by reverting to default
					output_filename = strdup("output.avi");
				} else {
					valid = false;
				}
			}
		}
		if (num_disparities % 16 || num_disparities < 0) {
			if (correct) {
				num_disparities = std::floor(((std::max(0,num_disparities))/16))*16;
			} else {
				valid = false;
			}
		}
		if (!((SAD_window_size % 2) && (SAD_window_size >=1))) {
			if (correct) {
				std::max(1, SAD_window_size % 2 ? SAD_window_size : SAD_window_size + 1);
			} else {
				valid = false;
			}
		}
		if (!(((p1 == 0) && (p2 == 0)) || ((p2 > 1) && (p1 > 0) && (p2 > p1)))) {
			if (correct) {
				p1 = std::max(0, p1);
				p2 = std::max(p1 > 0 ? p1+1 : 0, p2);
			} else {
				valid = false;
			}
		}
		
		// all of the rest of the variables are simple "greater than / equals"
		auto geq = [&](int& value1, int value2) {
			if (value1 < value2) {
				correct ? value1 = 0 : valid = false;
			}
		};
		geq(min_disparity, 0);
		geq(pre_filter_cap, 0);
		geq(uniqueness, 0);
		geq(disp12_max_diff, -1);
		geq(speckle_window_size, 0);
		geq(speckle_range, 0);
		return valid;
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
			if (!arguments->set_fourcc(arg)) { 
				argp_usage(state);
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
static char args_doc[] = "";

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
	//constructor initializes
	struct arguments arguments;
	//parse arguments
	argp_parse (&argp, argc, argv, 0, 0, &arguments);

	if (!arguments.is_valid(false)) {
		std::cerr << "Invalid argument settings. Attempting to correct them." << std::endl;
		if (arguments.is_valid(true)) {
			std::cerr << "Settings corrected. Continuing" << std::endl;
		} else {
			std::cerr << "Cannot correct settings. Exiting" << std::endl;
			return EXIT_FAILURE;
		} 
	}

	if (arguments.nogui) {
		cv::VideoCapture feed_src; //source video feed
		cv::VideoWriter  feed_dst; //destination video feed
		double input_width, split_width, input_height, input_fps, output_width, output_height, output_fps;
		//open iostreams and set dimension variables
		{
			bool valid = true;
			feed_src.open(arguments.input_filename);
			if (!feed_src.isOpened()) {
				std::cerr << "ERROR:\tInput file [" << arguments.input_filename << "] cannot be opened for reading" << std::endl;
				valid = false;
			} else {
				input_width = feed_src.get(CV_CAP_PROP_FRAME_WIDTH);
				input_height = feed_src.get(CV_CAP_PROP_FRAME_HEIGHT);
				input_fps = feed_src.get(CV_CAP_PROP_FPS);
				split_width = input_width * 0.5;
				
				output_width = split_width;
				output_height = input_height;
				output_fps = input_fps;

				feed_dst.open(arguments.output_filename, arguments.output_fourcc, output_fps, cv::Size(output_width, output_height), true);
				if (!feed_dst.isOpened()) {
					std::cerr << "ERROR:\tOutput file [" << arguments.output_filename << "] cannot be opened for writing" << std::endl;
					valid = false;
				}
			}
			if (!valid) {
				return EXIT_FAILURE;
			}
		}
		cv::Mat frame_src, left_eye, right_eye, frame_dst_16_gray, frame_dst_8_gray, frame_dst_8_colour;
		
		cv::StereoSGBM mapper(arguments.min_disparity, arguments.num_disparities, arguments.SAD_window_size, 
		                      arguments.p1, arguments.p2, arguments.disp12_max_diff, arguments.pre_filter_cap, 
		                      arguments.uniqueness, arguments.speckle_window_size, arguments.speckle_range,
		                      arguments.full_dp);
		
		//init first frame from VideoCapture
		//loop while there's current video frame data and nothing has been pressed
		//step prep next frame
		for (feed_src >> frame_src; frame_src.data && (cv::waitKey(33) < 0); feed_src >> frame_src) {
			left_eye = frame_src.colRange(0, split_width);
			right_eye = frame_src.colRange(split_width, input_width);

			mapper(left_eye, right_eye, frame_dst_16_gray);
			
			//the disparity mapper outputs CV_16UC1 when we need it in CV_8UC1
			frame_dst_16_gray.convertTo(frame_dst_8_gray, CV_8UC1);
			cvtColor(frame_dst_8_gray, frame_dst_8_colour, CV_GRAY2RGB);
			feed_dst << frame_dst_8_colour;
		}
	} else {
		std::cerr << "ERROR: gui not yet implemented. Try again with the --nogui argument" << std::endl;
	}

	return EXIT_SUCCESS;
}

