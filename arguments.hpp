#ifndef ARGUMENTS_HPP
#define ARGUMENTS_HPP

#include <mutex>
static std::mutex g_args_mutex;

/* This class is used by main to communicate with parse_opt. */
class Arguments
{
    public:
        Arguments();
        virtual ~Arguments();

        enum Arg {
            VERBOSE,
            NOGUI,
            OUTPUT_FOURCC,
            INPUT_FILENAME,
            OUTPUT_FILENAME,
            NUM_DISPARITIES,
            SAD_WINDOW_SIZE,
            MIN_DISPARITY,
            PRE_FILTER_CAP,
            UNIQUENESS,
            P1,
            P2,
            DISP12_MAX_DIFF,
            SPECKLE_WINDOW_SIZE,
            SPECKLE_RANGE,
            FULL_DP
        };

        const Arg arg_list[16] = {VERBOSE,
                                  NOGUI,
                                  OUTPUT_FOURCC,
                                  INPUT_FILENAME,
                                  OUTPUT_FILENAME,
                                  NUM_DISPARITIES,
                                  SAD_WINDOW_SIZE,
                                  MIN_DISPARITY,
                                  PRE_FILTER_CAP,
                                  UNIQUENESS,
                                  P1,
                                  P2,
                                  DISP12_MAX_DIFF,
                                  SPECKLE_WINDOW_SIZE,
                                  SPECKLE_RANGE,
                                  FULL_DP};

        void reset();
        bool is_valid(bool correct = false);
        bool is_valid(Arg arg, bool correct = false);



        template <typename Val>
        void set_value(Arg key, Val value) {
            g_args_mutex.lock(); // or, to be exception-safe, use std::lock_guard
            switch(key) {
                case VERBOSE:
                    try_set<bool, Val>(verbose, value);
                    break;
                case NOGUI:
                    try_set<bool, Val>(nogui, value);
                    break;
                case NUM_DISPARITIES:
                    try_set<int, Val>(num_disparities, value);
                    break;
                case SAD_WINDOW_SIZE:
                    try_set<int, Val>(SAD_window_size, value);
                    break;
                case INPUT_FILENAME:
                    try_set<std::string, Val>(input_filename, value);
                    break;
                case OUTPUT_FILENAME:
                    try_set<std::string, Val>(output_filename, value);
                    break;
                case OUTPUT_FOURCC:
                    //check if setting by int or by string
                    if (std::is_same<Val, int>::value) {
                        try_set<int, Val>(output_fourcc, value);
                    } else {
                        try_set_fourcc(value);
                    }
                    break;
                case MIN_DISPARITY:
                    try_set<int, Val>(min_disparity, value);
                    break;
                case PRE_FILTER_CAP:
                    try_set<int, Val>(pre_filter_cap, value);
                    break;
                case UNIQUENESS:
                    try_set<int, Val>(uniqueness, value);
                    break;
                case P1:
                    try_set<int, Val>(p1, value);
                    break;
                case P2:
                    try_set<int, Val>(p2, value);
                    break;
                case DISP12_MAX_DIFF:
                    try_set<int, Val>(disp12_max_diff, value);
                    break;
                case SPECKLE_WINDOW_SIZE:
                    try_set<int, Val>(speckle_window_size, value);
                    break;
                case SPECKLE_RANGE:
                    try_set<int, Val>(speckle_range, value);
                    break;
                case FULL_DP:
                    try_set<bool, Val>(full_dp, value);
                    break;
                default:
                    throw std::range_error("Error: unknown key");
                    break;
            }
            g_args_mutex.unlock();
        }

        template <typename T>
        T get_value(Arg key) {
            g_args_mutex.lock(); // or, to be exception-safe, use std::lock_guard
            T retval;
            switch(key) {
                case VERBOSE:
                    try_set<T, bool>(retval, verbose);
                    break;
                case NOGUI:
                    try_set<T, bool>(retval, nogui);
                    break;
                case OUTPUT_FOURCC:
                    try_set<T, int>(retval, output_fourcc);
                    break;
                case INPUT_FILENAME:
                    try_set<T, std::string>(retval, input_filename);
                    break;
                case OUTPUT_FILENAME:
                    try_set<T, std::string>(retval, output_filename);
                    break;
                case NUM_DISPARITIES:
                    try_set<T, int>(retval, num_disparities);
                    break;
                case SAD_WINDOW_SIZE:
                    try_set<T, int>(retval, SAD_window_size);
                    break;
                case MIN_DISPARITY:
                    try_set<T, int>(retval, min_disparity);
                    break;
                case PRE_FILTER_CAP:
                    try_set<T, int>(retval, pre_filter_cap);
                    break;
                case UNIQUENESS:
                    try_set<T, int>(retval, uniqueness);
                    break;
                case P1:
                    try_set<T, int>(retval, p1);
                    break;
                case P2:
                    try_set<T, int>(retval, p2);
                    break;
                case DISP12_MAX_DIFF:
                    try_set<T, int>(retval, disp12_max_diff);
                    break;
                case SPECKLE_WINDOW_SIZE:
                    try_set<T, int>(retval, speckle_window_size);
                    break;
                case SPECKLE_RANGE:
                    try_set<T, int>(retval, speckle_range);
                    break;
                case FULL_DP:
                    try_set<T, bool>(retval, full_dp);
                    break;
                default:
                    throw std::range_error("Error: unknown key");
                    break;
            }
            g_args_mutex.unlock();
            return retval;
        }

    private:
        template <typename Val>
        void try_set_fourcc(Val& value) {

            //lambda to check if a specific character is valid for fourcc input
            auto is_fourcc_char = [](char character) {
                //there are currently 3 fourcc codes that include the space character
                // ("RLE ", "Y16 ", "Y8  ")
                return isalnum(character) || isspace(character);
            };

            if (std::is_same<std::string, Val>::value) {
                std::string temp = ((std::string&) value);
                if (temp.length() == 4) {
                    if (is_fourcc_char(temp[0]) && is_fourcc_char(temp[1]) && is_fourcc_char(temp[2]) && is_fourcc_char(temp[3])) {
                        output_fourcc = CV_FOURCC(temp[0], temp[1], temp[2], temp[3]);
                    } else {
                        throw std::runtime_error("Error: FOURCC code contains illegal characters");
                    }
                } else {
                    throw std::runtime_error("Error: output_fourcc value is not 4 characters long");
                }
            } else {
                throw std::runtime_error("Error: output_fourcc value is not a string");
            }
        }

        template <typename Var, typename Val>
        void try_set(Var& var, Val& val) {
            if (std::is_same<Var, Val>::value) {
                var = (Var&)(val);
            } else {
                throw std::runtime_error("Error: Variable and Value are not of the same type");
            }
        }

        bool verbose;
        bool nogui;
        int output_fourcc;
        std::string input_filename;
        std::string output_filename;
        std::string output_filename_default;
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
