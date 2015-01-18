#include "opencv2/highgui/highgui.hpp" //CV_FOURCC
#include "arguments.hpp"

Arguments::Arguments() {
	reset();
}

Arguments::~Arguments() {
	if (input_filename) {
		delete input_filename;
		input_filename = NULL;
	}
	if (output_filename) {
		delete output_filename;
		output_filename = NULL;
	}
}

void Arguments::reset() {
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


bool Arguments::set_fourcc(const char* code) {
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

bool Arguments::is_valid(bool correct) {
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

