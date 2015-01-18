#ifndef ARGUMENTS_HPP
#define ARGUMENTS_HPP

/* This class is used by main to communicate with parse_opt. */
class Arguments
{
public:
	Arguments();
	virtual ~Arguments();

	void reset();
	bool set_fourcc(const char* code);
	bool is_valid(bool correct = false);

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


#endif//ARGUMENTS_HPP
