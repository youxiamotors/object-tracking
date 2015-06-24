all: driveassist compare

CFLAGS := -O0 -Wall -g
OPENCV_FLAGS := -lopencv_core  -lopencv_highgui -lopencv_imgproc  -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect   -lopencv_legacy

driveassist: driveassist.cpp *.hpp driveassist.cpp feature_*.cpp
	g++  $(CFLAGS) $(OPENCV_FLAGS)  -o "driveassist" "driveassist.cpp" feature_*.cpp

compare: compare.cpp
	g++ $(CFLAGS) $(OPENCV_FLAGS) -o "compare" compare.cpp

clean: 
	rm -rfv driveassist
