all: driveassist

CFLAGS := -O0 -Wall -g

driveassist: driveassist.cpp *.hpp *.cpp
	g++  $(CFLAGS) -lopencv_core  -lopencv_highgui -lopencv_imgproc  -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect   -lopencv_legacy  -o "driveassist" "driveassist.cpp" feature_meanfunc.cpp

clean: 
	rm -rfv driveassist
