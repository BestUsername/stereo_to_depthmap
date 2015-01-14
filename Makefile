########LIBRAIRIES
LIBS_ffmpeg = -lm -lz -lpthread -lavformat -lavcodec -lavutil

LIBS_opencv = -lopencv_core -lopencv_calib3d -lopencv_highgui -lopencv_imgproc -lopencv_video -lopencv_objdetect

LIBS = $(LIBS_ffmpeg) $(LIBS_opencv)

CXXFLAGS = -std=c++11 -g -O0 -Wall

all:
	g++ -o test test.cpp $(CXXFLAGS) $(LIBS)

