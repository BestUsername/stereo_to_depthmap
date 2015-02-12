#include <QApplication>
#include <QException>
#include <QDebug>

#include "opencv2/highgui/highgui.hpp" //CV_FOURCC
#include "opencv2/imgproc/imgproc.hpp" //CV_Gray2RGB cvtColor
#include "opencv2/calib3d/calib3d.hpp" //StereoSGBM

#include <argp.h>
#include <iostream> //cerr

#include "arguments.hpp"
#include "processor.h"
#include "qtopencvdepthmap.h"

const char* argp_program_version = "stereo_to_depthmap 0.1";
const char* argp_program_bug_address = "<bugs@marc.zone>";

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
{"startFrame"       ,    's',   "INDEX", 0,                                               "Optional starting frame for clip processing. Default 0.", 1},
{"endFrame"         ,    'e',   "INDEX", 0,                                                 "Optional ending frame for clip processing. Default 0.", 1},
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
{0                  ,      0,         0, 0,                                                                                                       0, 0}
};

/*
PARSER. Field 2 in ARGP.
Order of parameters: KEY, ARG, STATE.
*/
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    Arguments *arguments = (Arguments*)(state->input);

    switch (key)
    {
        //group 0 - ungrouped
        case 'v': //verbose
            arguments->set_value<bool>(Arguments::VERBOSE, true);
            break;
        case 'c': //nogui
            arguments->set_value<bool>(Arguments::NOGUI, true);
            break;

        //group 1 - input/output
        case 'f': //fourcc
            arguments->set_value<std::string>(Arguments::OUTPUT_FOURCC, std::string(arg));
            break;
        case 'i': //infile
            arguments->set_value<std::string>(Arguments::INPUT_FILENAME, std::string(arg));
            break;
        case 'o': //outfile
            arguments->set_value<std::string>(Arguments::OUTPUT_FILENAME, std::string(arg));
            break;
        case 's': //starting frame number
            arguments->set_value<int>(Arguments::START_FRAME, std::stoi(arg));
            break;
        case 'e': //ending frame number
            arguments->set_value<int>(Arguments::END_FRAME, std::stoi(arg));
            break;

        //group 2 - information shared between StereoSGBM and StereoBM
        case 'd': //disparity
            arguments->set_value<int>(Arguments::NUM_DISPARITIES, std::stoi(arg));
            break;
        case 'w': //SAD_window_size
            arguments->set_value<int>(Arguments::SAD_WINDOW_SIZE, std::stoi(arg));
            break;
            //group 3 - information specific to StereoSGBM
        case 'm': //min_disparity
            arguments->set_value<int>(Arguments::MIN_DISPARITY, std::stoi(arg));
            break;
        case 't': //truncate
            arguments->set_value<int>(Arguments::PRE_FILTER_CAP, std::stoi(arg));
            break;
        case 'u': //uniqueness
            arguments->set_value<int>(Arguments::UNIQUENESS, std::stoi(arg));
            break;
        case 1000: //P1
            arguments->set_value<int>(Arguments::P1, std::stoi(arg));
            break;
        case 1001: //P2
            arguments->set_value<int>(Arguments::P2, std::stoi(arg));
            break;
        case 1002: //maxDiff
            arguments->set_value<int>(Arguments::DISP12_MAX_DIFF, std::stoi(arg));
            break;
        case 1003: //speckleWindowSize
            arguments->set_value<int>(Arguments::SPECKLE_WINDOW_SIZE, std::stoi(arg));
            break;
        case 1004: //speckleRange
            arguments->set_value<int>(Arguments::SPECKLE_RANGE, std::stoi(arg));
            break;
        case 1005: //fullDP
            arguments->set_value<int>(Arguments::FULL_DP, true);
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
static char doc[] = "stereo_to_depthmap -- A program to calculate a depth map video from a 3D Side-By-Side video.";

/*
The ARGP structure itself.
*/
static struct argp argp = {options, parse_opt, args_doc, doc, 0, 0, 0};


int main( int argc, char** argv ) {
    int retval = EXIT_SUCCESS;

    //constructor initializes
    Arguments arguments;
    //parse arguments
    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    if (!arguments.is_valid(false)) {
        std::cerr << "Invalid argument settings. Attempting to correct them." << std::endl;
        if (arguments.is_valid(true)) {
            std::cerr << "Settings corrected. Continuing" << std::endl;
        } else {
            std::cerr << "Cannot correct settings. Exiting" << std::endl;
            retval = EXIT_FAILURE;
        }
    }

    if (EXIT_SUCCESS == retval) {
        if (arguments.get_value<bool>(Arguments::NOGUI)) {
            cv::VideoCapture feed_src; //source video feed

                std::string input_filename = arguments.get_value<std::string>(Arguments::INPUT_FILENAME);
                feed_src.open(input_filename);
                if (!feed_src.isOpened()) {
                    std::cerr << "ERROR:\tInput file [" << input_filename << "] cannot be opened for reading" << std::endl;
                    retval = EXIT_FAILURE;
                } else {
                    Processor::process_clip(feed_src, arguments);

                }
        } else {

            int res=-1;

            try {
                QApplication a(argc, argv);
                QtOpenCVDepthmap w(arguments, 0);
                w.show();

                res = a.exec();
            }
            catch(QException &e) {
                qCritical() << QString("Exception: %1").arg( e.what() );
            }
            catch(...) {
                qCritical() << QString("Unhandled Exception");
            }

            retval = res == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    return retval;
}

