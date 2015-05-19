#include <stdio.h>
#include <stdint.h>
#include <opencv2/opencv.hpp>
#include <opencv2/legacy/legacy.hpp>

#include "driveassist.hpp"

#include "feature_meanfunc.hpp"

#define LANE_USE_KALMAN 1

/// 高速路
/*
#define SRC "file:///media/TOURO/PAPAGO/94750223/16290029.MOV"
int roiX = 195 * 2;
int roiY = 258 * 2;
int roiWidth = 384 * 2;
int roiHeight = 99 * 2;
int srcX1 = 161 * 2;
int KFStateL = 335;
int KFStateR = 450;
*/

/*
/// 城西路
#define SRC "file:///media/TOURO/PAPAGO/94750223/17050041.MOV"
int roiX = 195 * 2;
int roiY = 258 * 2;
int roiWidth = 384 * 2;
int roiHeight = 99 * 2;
int srcX1 = 161 * 2;
int KFStateL = 335;
int KFStateR = 450;
*/



/// 城北路
/*
#define SRC "file:///media/TOURO/PAPAGO/190CRASH/13510002.MOV"

int roiX = 300;
int roiY = 258 * 2;
int roiWidth = 384 * 2;
int roiHeight = 99 * 2;
int srcX1 = 303;
int KFStateL = 315;
int KFStateR = 445;

int mmppx = 100;    /// IPM 图中每个像素的长度，单位：毫米
int carOriginX = 380;    /// imgIPM32 图中汽车前脸的坐标
int carOriginY = 200;    /// imgIPM32 图中汽车前脸的坐标
int carROIX = 985 / 1.5;
int carROIY = 665 / 1.5;
int carROIWidth = 80 / 1.5;
int carROIHeight = 80 /1.5;
*/



//#define SRC "file:///media/TOURO/PAPAGO/95450225/17500006.MOV" /** 车辆 */


using namespace std;
using namespace cv;
using namespace ot;

/*
int roiX = 300; //195 * 2;
int roiY = 258 * 2;
int roiWidth = 384 * 2;
int roiHeight = 99 * 2;
int srcX1 = 303; //161 * 2;
*/

/// 手动选择的高斯核
int sigmaX = 4;
int sigmaY = 25;
int gaussianSize = 25;

/*
/// 根据道路标线长度标定的高斯核
int sigmaX = 6;
int sigmaY = 47;
int gaussianSize = 6
*/
int thresholdingQ = 975;
int peakFilterAlpha = 90;
int groupingThreshold = 50;
int ransacIterNum = 50;



/**
 * 车辆探测和测距使用的参数
 */
/**
int mmppx = 100;    /// IPM 图中每个像素的长度，单位：毫米
int carOriginX = 380;    /// imgIPM32 图中汽车前脸的坐标
int carOriginY = 200;    /// imgIPM32 图中汽车前脸的坐标
int carROIX = 985 / 1.5;
int carROIY = 665 / 1.5;
int carROIWidth = 80 / 1.5;
int carROIHeight = 80 /1.5;
*/

#define SRC "file:///media/TOURO/PAPAGO/431CRASH/09230001.MOV"
int mmppx = 100;    /// IPM 图中每个像素的长度，单位：毫米
int carOriginX = 380;    /// imgIPM32 图中汽车前脸的坐标
int carOriginY = 200;    /// imgIPM32 图中汽车前脸的坐标
int carROIX = 1088 / 1.5;
int carROIY = 542 / 1.5;
int carROIWidth = 140 / 1.5;
int carROIHeight = 140 /1.5;



const char *winOrigin = "原图";
const char *winROI = "感兴趣区域";
const char *winGray = "灰度图";
const char *winFeature = "目标特征";


Mat frame, imgOrigin, imgROI, imgGray, imgIPM, imgIPM32, imgGaussian, imgThreshold, imgThresholdOld;



void followObject(Mat& iInput, Rect _roi) {
    static ObjectTracking *obj = NULL;
    if (obj == NULL) {
        obj = new ObjectTracking(iInput, _roi);
    
        /// 绘制原始图像
        imshow("原始跟踪图像", Mat(iInput, _roi));
        
        fprintf(stderr, "对象追踪初始化完毕\n");
    }
    
    
    /// 计算频率
    static double t1 = getMicroTime();
    double elapse = getMicroTime() - t1;
    t1 = getMicroTime();
    
    char txt[1024] = {0};
    snprintf(txt, sizeof(txt) - 1, "%0.1lf Hz", 1.0 / elapse);
    putText(iInput, String(txt), Point(10, iInput.rows - 21), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
    putText(iInput, String(txt), Point(10, iInput.rows - 19), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(255, 255, 255), 1, CV_AA);
    putText(iInput, String(txt), Point(10, iInput.rows  -20), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 0, 255), 1, CV_AA);

    
    obj->detect(iInput);
    
    Rect rctObj = obj->getObjectRect();
    
    /// 绘制探测到的目标窗口
    rectangle(imgOrigin, Point(rctObj.x, rctObj.y), Point(rctObj.x + rctObj.width, rctObj.y + rctObj.height), CV_RGB(128, 128, 255), 2);
    
    
    /// 在小窗口中绘制探测到的图像
    //imshow("探测到的对象", Mat(imgOrigin, rctObj));
    
}







int main()
{
    int frameIdx = 0;
    
    //namedWindow(winConfig, CV_WINDOW_KEEPRATIO | CV_WINDOW_NORMAL);
    //createTrackbar("src Y", winGray, &srcY, 480, NULL);
    
    
    VideoCapture capVideo(SRC);
    
    if (!capVideo.isOpened()) {
        fprintf(stderr, "Could not open video");
        exit(-1);
    }
    
    double width, height;
    width = capVideo.get(CV_CAP_PROP_FRAME_WIDTH);
    height = capVideo.get(CV_CAP_PROP_FRAME_HEIGHT);
    fprintf(stderr, "Video size: %lfx%lf\n", width, height);
    
    
    VideoWriter oVideoWriter ("/media/TOURO/ObjectTracking.avi", CV_FOURCC('P','I','M','1'), 30, Size(width / 1.5, height / 1.5), true);
    
        
    
    
    /// 开始处理
    int start = 0;
    
    while (1) {
        if (start < 2) {
            waitKey(27);
            start += 1;
        }
        
        capVideo >> frame;  
        

        if (frame.empty()) {
            break;
        }
        else {
            frameIdx++;
            //fprintf(stderr, "Processing frame %d", frameIdx);
        }
        
        
        /// 原图
        resize(frame, imgOrigin, Size(frame.cols / 1.5, frame.rows / 1.5));
        
        
        Mat imgClone = imgOrigin.clone();

        Rect roiCar = Rect(carROIX, carROIY, carROIWidth, carROIHeight);
        followObject(imgClone, roiCar);
            

        imshow(winOrigin, imgOrigin);
        
        oVideoWriter.write(imgClone);

        
        waitKey(10);
    }
    
    
    
	return 0;
}
