#include <stdio.h>
#include <stdint.h>
#include <typeinfo>
#include <opencv2/opencv.hpp>
#include <opencv2/legacy/legacy.hpp>



using namespace std;
using namespace cv;



int main(int argc, char* argv[])
{
    char *vidSrc1, *vidSrc2, *vidDst;
    
    if (argc < 3) {
        fprintf(stderr, "%s <src1> <src2> <dst>\n", argv[0]);
        exit(-1);
    }
    
    vidSrc1 = argv[1];
    vidSrc2 = argv[2];
    vidDst = argv[3];
    
    
    VideoCapture capSrc1(vidSrc1);
    VideoCapture capSrc2(vidSrc2);
    VideoCapture capDst(vidDst);
    
    if (!capSrc1.isOpened()) {
        fprintf(stderr, "Could not open fisrt source video\n");
        exit(-1);
    }
    if (!capSrc2.isOpened()) {
        fprintf(stderr, "Could not open second source video\n");
        exit(-1);
    }

    
    double width, height;
    width = capSrc1.get(CV_CAP_PROP_FRAME_WIDTH);
    height = capSrc2.get(CV_CAP_PROP_FRAME_HEIGHT);
    fprintf(stderr, "First source video size: %lfx%lf\n", width, height);
    
    
    VideoWriter oVideoWriter (vidDst, CV_FOURCC('P','I','M','1'), 30, Size(width, height), true);
    //VideoWriter oVideoWriter (vidDst, CV_FOURCC('X', 'V', 'I', 'D'), 30, Size(width, height), true);
    //VideoWriter oVideoWriter (vidDst, CV_FOURCC('M', 'J', 'P', 'G'), 30, Size(width, height), true);
    
    if (!oVideoWriter.isOpened()) {
        fprintf(stderr, "Could not open dst file for write\n");
        exit(-2);
    }
    
    
    Mat imgDst, imgSrc1, imgSrc2;
    
    Rect roiSrc(0, height / 4, width, height / 2);
    Rect roiDst1(0, 0, width, height / 2);
    Rect roiDst2(0, height / 2, width, height / 2);
    
    
    /// 开始处理
    int frameIdx = 0;
    while (1) {
        capSrc1 >> imgSrc1; 
        capSrc2 >> imgSrc2;
        
        if (imgDst.empty()) {
            fprintf(stderr, "初始化目标图像\n");
            imgDst = imgSrc1.clone();
            imgDst *= 0;
        }
        
        if (imgSrc1.empty() && imgSrc2.empty()) {
            break;
        }
        else {
            frameIdx++;
            //fprintf(stderr, "Processing frame %d", frameIdx);
        }
        
        if (imgSrc1.empty()) {
            imgSrc1 = imgDst.clone();
            imgSrc1 *= 0;
        }
        if (imgSrc2.empty()) {
            imgSrc2 = imgDst.clone();
            imgSrc2 *= 0;
        }
        
        
        
        /// 取第一个视频的中间部分，以及第二个视频的中间部分，然后合成一个视频
        imgSrc1 = Mat(imgSrc1, roiSrc);
        imgSrc2 = Mat(imgSrc2, roiSrc);
        
        
        putText(imgSrc1, String(basename(vidSrc1)), Point(10, 21), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
        putText(imgSrc1, String(basename(vidSrc1)), Point(10, 19), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
        putText(imgSrc1, String(basename(vidSrc1)), Point(10, 20), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 0, 255), 1, CV_AA);
        
        putText(imgSrc2, String(basename(vidSrc2)), Point(10, 21), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
        putText(imgSrc2, String(basename(vidSrc2)), Point(10, 19), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
        putText(imgSrc2, String(basename(vidSrc2)), Point(10, 20), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 0, 255), 1, CV_AA);
        
        
        
        
        imgSrc1.copyTo(Mat(imgDst, roiDst1));
        imgSrc2.copyTo(Mat(imgDst, roiDst2));
        
        
        imshow("", imgDst);
        oVideoWriter.write(imgDst);

        fprintf(stderr, "Processed %d frmaes\r", frameIdx);
        
        waitKey(10);
    }
    
    
    
	return 0;
}
