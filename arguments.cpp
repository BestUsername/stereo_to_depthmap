#include <stdexcept>

#include "opencv2/highgui/highgui.hpp" //CV_FOURCC
#include "arguments.hpp"



Arguments::Arguments() {
    output_filename_default = "output.avi";
    reset();
}

Arguments::~Arguments() {
}

void Arguments::reset() {
    g_args_mutex.lock(); // or, to be exception-safe, use std::lock_guard
    verbose = false;
    nogui = false;
    output_fourcc = CV_FOURCC_DEFAULT;
    input_filename = "";
    output_filename = output_filename_default;
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
    g_args_mutex.unlock();
}

bool Arguments::is_valid(bool correct) {
    bool invalid = false;

    for (const Arg& a : arg_list) {
        invalid = invalid ||!is_valid(a, correct);
    }

    return !invalid;
}

bool Arguments::is_valid(const Arg &arg, bool correct) {
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

    // a lot of the variables are simple "greater than / equals" tests
    auto geq = [&](int& value1, int value2) {
        if (value1 < value2) {
            correct ? value1 = 0 : valid = false;
        }
    };

    g_args_mutex.lock(); // or, to be exception-safe, use std::lock_guard

    switch(arg) {
        case VERBOSE:
        case FULL_DP:
        case OUTPUT_FOURCC:
            break;
        case NOGUI:
            if (nogui) {
                if (input_filename.empty()) {
                    //can't correct this without using stdin/stdout
                    valid = false;
                }
                if (output_filename.empty()) {
                    if (correct) {
                        //this can be corrected by reverting to default
                        output_filename = output_filename_default;
                    } else {
                        valid = false;
                    }
                }
            }
            break;
        case OUTPUT_FILENAME:
            if (nogui) {
                if (output_filename.empty()) {
                    if (correct) {
                        //this can be corrected by reverting to default
                        output_filename = output_filename_default;
                    } else {
                        valid = false;
                    }
                }
            }
            break;
        case INPUT_FILENAME:
            if (nogui) {
                if (input_filename.empty()) {
                    //can't correct this without using stdin/stdout
                    valid = false;
                }
            }
            break;
        case NUM_DISPARITIES:
            if (num_disparities % 16 || num_disparities < 0) {
                if (correct) {
                    num_disparities = std::floor(((std::max(0,num_disparities))/16))*16;
                } else {
                    valid = false;
                }
            }
            break;
        case SAD_WINDOW_SIZE:
            if (!((SAD_window_size % 2) && (SAD_window_size >=1))) {
                if (correct) {
                    std::max(1, SAD_window_size % 2 ? SAD_window_size : SAD_window_size + 1);
                } else {
                    valid = false;
                }
            }
            break;
        case MIN_DISPARITY:
            geq(min_disparity, 0);
            break;
        case PRE_FILTER_CAP:
            geq(pre_filter_cap, 0);
            break;
        case UNIQUENESS:
            geq(uniqueness, 0);
            break;
        case P1: //P1 and P2 have to be validated against each other
        case P2:
            if (!(((p1 == 0) && (p2 == 0)) || ((p2 > 1) && (p1 > 0) && (p2 > p1)))) {
                if (correct) {
                    p1 = std::max(0, p1);
                    p2 = std::max(p1 > 0 ? p1+1 : 0, p2);
                } else {
                    valid = false;
                }
            }
            break;
        case DISP12_MAX_DIFF:
            geq(disp12_max_diff, -1);
            break;
        case SPECKLE_WINDOW_SIZE:
            geq(speckle_window_size, 0);
            break;
        case SPECKLE_RANGE:
            geq(speckle_range, 0);
            break;
        default:
            throw std::range_error("Error: Unknown variable index");
    }
    g_args_mutex.unlock();
    return valid;
}
