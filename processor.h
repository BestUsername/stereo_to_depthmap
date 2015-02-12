#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "opencv2/highgui/highgui.hpp" //VideoCapture
#include "arguments.hpp"

class Processor
{
public:
    static void process_clip(cv::VideoCapture& feed_src, Arguments& arguments);
};

#endif // PROCESSOR_H
