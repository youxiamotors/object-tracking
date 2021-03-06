#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

#include <opencv2/opencv.hpp>

#include "feature_base.hpp"
#include "feature_meanfunc.hpp"
#include "feature_svd.hpp"
#include "feature_hog.hpp"

using namespace std;
using namespace cv;

namespace cv {
    typedef Rect_<float> Rectf;
}


namespace ot {


/**
 * 颜色直方图特征
 */
class ColorHist : public FeatureBase {
    protected:
        int bins;
        MatND hist;
    
    public:
        ColorHist() {};
        
        ColorHist(Mat& _input, Rect roi) {
            bins = 16;
            
            fixROI(roi, _input);
            
            Mat img = Mat(_input, roi);
            calc(img);
        }
    
        ~ColorHist() {}
        
    
        /**
         * 计算输入图片的特征
         */
        void calc(Mat& img) {
            Mat iGray;
            //cvtColor(img, iGray, CV_RGB2HSV);    /// 在外部已经做过转换了，不需要每次都转换，这样可以提升性能
            iGray = img;
                        
            int hist_size[] = {bins, bins};
            float hrange[] = {0, 180};  
            float srange[] = {0, 256};  
            const float *ranges[] = {hrange, srange};  
            int channels[] = {0, 1};

            /*
            static int i = 0;
            char path[1024] = {0};
            snprintf(path, sizeof(path) - 1, "/media/TOURO/hsv/inner/%06d.png", ++i);
            imwrite(path, iGray);
            fprintf(stderr, "Written to %s\n", path);
            */

            calcHist( &iGray, 1, channels, Mat(), // do not use mask
                hist, 2, hist_size, ranges,
                true, // the histogram is uniform
                false );
                
            
            
            /// 转换为概率密度分布
            normalize(hist, hist);
            
            /*
            assert(hist.rows > 0);


            
            double max_val;
            minMaxLoc(hist, 0, &max_val, 0, 0);
            int scale = 2;
            int hist_height=256;
            Mat hist_img = Mat::zeros(hist_height,bins*scale, CV_8UC3);
            for(int i=0; i < bins; i++)
            {
                float bin_val = hist.at<float>(i); 
                int intensity = cvRound(bin_val*hist_height/max_val);  //要绘制的高度
                rectangle(hist_img,Point(i*scale,hist_height-1),
                    Point((i+1)*scale - 1, hist_height - intensity),
                    CV_RGB(255,255,255));
            }
            
            imshow( "Gray Histogram", hist_img );
            */
        }

        
        /**
         * 计算输入特征是当前特征的概率
         */
        double prob(ColorHist& feature) {
            double p;
            
            
            /// 欧氏距离
            /**
            for (i = 0; i < bins; i++) {
                theta = feature.hist.at<float>(i) - hist.at<float>(i);                
                theta *= theta;
                dist += theta;
            }
            dist = sqrt(dist);
            
            p = exp(-0.00001 * dist);
            */
            
            
            /// Bhattacharyya 系数
            /**
            for (i = 0; i < bins; i++) {
                p += sqrt(feature.hist.at<float>(i) * hist.at<float>(i));
            }
            */
            
            
            p = compareHist(feature.hist, hist, CV_COMP_BHATTACHARYYA);
            p = exp(-20.0 * (p * p));
            p = isnan(p) ? 0 : p;
            
            
            return p;
        }
        
        void show(const char* name) {
        }
};

    
/**
 * RGB 直方图特征
 */
class RGBHist : public FeatureBase {
    protected:
        static const int rbins = 8;
        static const int gbins = 8;
        static const int bbins = 8;
        static const int dims = 3;
        Mat hist;
        
    public:
        RGBHist() {};
        ~RGBHist() {};
        
        RGBHist(Mat& _input, Rect roi) {
            fixROI(roi, _input);
            
            Mat iROI = Mat(_input, roi);
            calc(iROI);
        }
        

        void calc(Mat& _input) {   
            Mat img = _input;
        
            int hist_size[] = {rbins, gbins, bbins};
            float rrange[] = {0, 255};  
            float grange[] = {0, 255};  
            float brange[] = {0, 255};  
            const float *ranges[] = {rrange, grange, brange};  
            int channels[] = {0, 1, 2};

            calcHist( &img, 1, channels, Mat(), hist, dims, hist_size, ranges, true, false );
            normalize(hist, hist);
        }
        
        double prob(RGBHist& feature) {
            double p;
            float bc;
            
            
            bc = compareHist(feature.hist, hist, CV_COMP_BHATTACHARYYA);    /// 返回值区间为 [0, 1]
            p = 0;

            if (bc != 1.f) {
                p = exp((-20.f) * (bc * bc));
            }
            
            return p;
        }
};



/**
 * 目标跟踪类
 * 
 * 使用粒子滤波对目标进行跟踪，概率使用目标的图形特征进行计算。
 * 可以使用构造函数中的 regions 参数来计算多个子区域的特征，并取子区域特征的概率的平均值作为最终概率
 */
template <class T>
class ObjectTracking {
    public: 
        /**
         * 构造函数
         * 
         * @param Mat&      输入图像
         * @param Rect      要跟踪的目标的位置
         * @param vector<Rect_<float>>*  要跟踪的目标的特征子区域（相对于 ROI 区域，使用百分比坐标），如果传入 NULL，则不使用子区域
         */
        ObjectTracking(Mat& _input, Rect roi, vector<Rect_<float> > *regions) {
            objRect = roi;
            
            subROI.clear();
            if (regions == NULL) {
                subROI.push_back(Rect(0, 0, 1, 1));
            }
            else {
                unsigned int i;
                for (i = 0; i < regions->size(); i++) {
                    subROI.push_back((*regions)[i]);
                }
            }
            
            fprintf(stderr, "使用了 %lu 个子特征\n", subROI.size());
            

            Mat iPattern;
            iPattern = _input.clone();
            updateSubRegions(iPattern, roi);
            oldFeatures = features;
            
            
            /// 初始化粒子滤波
            conStateNum = 3;
            conMeasureNum = 3;
            conSampleNum = 200;
            con = cvCreateConDensation(conStateNum, conMeasureNum, conSampleNum);
            
            lowerBound = cvCreateMat(conStateNum, 1, CV_32F);
            upperBound = cvCreateMat(conStateNum, 1, CV_32F);
            
            cvmSet(lowerBound, 0, 0, 0.0);
            cvmSet(lowerBound, 1, 0, 0.0);
            cvmSet(lowerBound, 2, 0, 0.0);
            
            cvmSet(upperBound, 0, 0, 30);
            cvmSet(upperBound, 1, 0, 30);
            cvmSet(upperBound, 2, 0, 20);
            float A[3][3] = {
                1, 0, 0, 
                0, 1, 0,
                0, 0, 1
            };
            memcpy(con->DynamMatr, A, sizeof(A));
            
            
            cvConDensInitSampleSet(con, lowerBound, upperBound);
            
            //CvRNG rng_state = cvRNG(time(NULL));
            for(int i = 0; i < conSampleNum; i++){
                //con->flSamples[i][0] = float(cvRandInt( &rng_state ) % _input.cols);    // X
                //con->flSamples[i][1] = float(cvRandInt( &rng_state ) % _input.rows);    // Y
                //con->flSamples[i][2] = float(cvRandInt( &rng_state ) % _input.cols);  // height
                con->flSamples[i][0] = roi.x;
                con->flSamples[i][1] = roi.y;
                con->flSamples[i][2] = roi.width;
            }
        }
    
    
        ~ObjectTracking() {
            features.clear();
            oldFeatures.clear();
            
            free(lowerBound);
            free(upperBound);
        }
    
        
        /**
         * 使用给定的图像，更新（或创建）子区域的特征
         * 
         * @param Mat&  指定的原图
         * @param Rect  感兴趣区域
         */
        void updateSubRegions(Mat& img, Rect roi) {
            Mat iROI = Mat(img, roi);
            vector<Rect> r;
            
            this->subROITransform(iROI, r);
            
            features.clear();
            for (unsigned int i = 0; i < r.size(); i++) {
                //FeatureBase::fixROI(r[i], iROI);
                
                char title[1024] = {0};
                sprintf(title, "子区域 %u", i);
                imshow(title, Mat(iROI, r[i]));
                
                features.push_back(T(iROI, r[i]));
            }
        }
        
        
        /**
         * 将按比例保存的 subROI 转换成绝对坐标 
         * 
         * @param const Mat&    输入图像，转换后的 ROI 将以该图像的大小作为基准
         * @param vector<Rect>& 转换后的 ROI
         */
        void subROITransform(const Mat& img, vector<Rect> &roi) {
            unsigned int i;
            
            roi.clear();
            for (i = 0; i < subROI.size(); i++) {
                Rect r;
                r.x = subROI[i].x * img.cols;
                r.y = subROI[i].y * img.rows;
                r.width = subROI[i].width * img.cols + 1;
                r.height = subROI[i].height * img.rows + 1;
                
                if (r.x + r.width > img.cols) {
                    r.width = img.cols - r.x;
                }
                if (r.y + r.height > img.rows) {
                    r.height = img.rows - r.y;
                }
                
                assert(r.width > 0 && r.height > 0 && r.x >= 0 && r.y >= 0);
                
                roi.push_back(r);
            }
            
            assert(subROI.size() == roi.size());
        }
        
    
    protected:
        vector<T> features, oldFeatures;
        Rect objRect;
        vector<Rectf> subROI;        /** 子区域 ROI，以百分比保存，比如 (0, 0, 1, 1) 就代表整张图片，(0, 0, 0.5, 0.5) 就代表图片左上部分 **/
        CvConDensation *con;
        int conSampleNum, conStateNum, conMeasureNum;
        CvMat *lowerBound, *upperBound;
        
    public: 
    
        /**
         * 进行一次探测：对粒子进行重采样
         */
        void detect(Mat& _input) {
            /*
            if (oldFeature != NULL) {
                delete oldFeature;
            }
            oldFeature = feature;
            */
            
            /// 开始粒子滤波
            double maxp = 0;
            double minp = 1;
            
            Mat iPattern;
            //cvtColor(_input, iPattern, CV_RGB2HSV);
            iPattern = _input.clone();
            //cvtColor(_input.clone(), _input, CV_RGB2HSV);
            
            //#pragma omp parallel for
            for (int i = 0; i < conSampleNum; i++) {
                int x, y, width;
                double p;
                
                
                x = con->flSamples[i][0];
                y = con->flSamples[i][1];
                width = con->flSamples[i][2];
                /*
                x = objRect.x;
                y = objRect.y;
                width = objRect.width;
                */
                
                Rect r = Rect(x, y, width, width);
                FeatureBase::fixROI(r, iPattern);
                p = prob(Mat(iPattern, r));
                
                
                con->flConfidence[i] = p;
                if (p > maxp) {
                    maxp = p;
                    //fprintf(stderr, "max p = %0.2f, pos = (%d, %d)\n", p * 100, x, y);
                }
                if (p < minp) {
                    minp = p;
                }
                
                /// 在原图中绘制出粒子
                circle(_input, Point(x, y), 1, CV_RGB(255, 0, 0));
            }
            
            
            
            
            /// 概率热力图
            /*
            Mat t;
            cvtColor(_input, t, CV_RGB2GRAY);
            for (int x = 0; x < _input.cols; x += 1) {
                for (int y = 0; y < _input.rows; y += 1) {
                    T featureTmp = T(iPattern, Rect(x, y, objRect.width, objRect.height));
                    double p = oldFeature->prob(featureTmp);
                    p *= 255;
                    
                    fprintf(stderr, "(%3d, %3d) prob=%lf\n", x, y, p);
                    t.ptr<unsigned char>(y)[x] = (int)p;
                }
            }
            imshow("Heat Map", t);
            imwrite("/media/TOURO/heatmap-hog.png", t);
            */
            

            
            cvConDensUpdateByTime(con);
            
            
            
            objRect.x = con->State[0];
            objRect.y = con->State[1];
            objRect.width = objRect.height = con->State[2];
            
            
            char txt[1024] = {0};
            double p_target;
            Mat imgTmp2 = Mat(iPattern, Rect(objRect.x, objRect.y, objRect.width, objRect.height));
            
            p_target = prob(imgTmp2);
            
            snprintf(txt, sizeof(txt) - 1, "Max & Min Prob: %.02f%%  %0.02f%%", maxp * 100, minp * 100);
            putText(_input, String(txt), Point(11, 21), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(9, 19), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(10, 20), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 255, 0), 1, CV_AA);
            
            
            snprintf(txt, sizeof(txt) - 1, "Target Prob: %0.2f%%", p_target * 100);
            putText(_input, String(txt), Point(11, 41), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(9, 39), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(10, 40), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 255, 0), 1, CV_AA);
            
            
            snprintf(txt, sizeof(txt) - 1, "Target Pos: (%d, %d), width: %d", objRect.x, objRect.y, objRect.width);
            putText(_input, String(txt), Point(11, 61), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(9, 59), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(128, 128, 128), 1, CV_AA);
            putText(_input, String(txt), Point(10, 60), CV_FONT_HERSHEY_DUPLEX, 0.7, CV_RGB(0, 255, 0), 1, CV_AA);
            
            
            circle(_input, Point(con->State[0], con->State[1]), 2, CV_RGB(0, 255, 0));
            
            Scalar col = CV_RGB(255, 255, 0);
            if (p_target >= 0.9) {
                col = CV_RGB(0, 255, 0);
            }
            else if (p_target < 0.8) {
                col = CV_RGB(255, 0, 0);
            }
            rectangle(_input, Point(objRect.x, objRect.y), Point(objRect.x + objRect.width, objRect.y + objRect.height), col, 2);
            
            //_waitKey();
        };
        
        
        /**
         * 对一个给定的区域，计算该区域与原始特征的概率
         */
        double prob(Mat img) {
            assert(oldFeatures.size() == subROI.size());
            assert(subROI.size() > 0);
            
            vector<Rect> r;
            unsigned int i;
            double p = 1;
            
            this->subROITransform(img, r);
            
            for (i = 0; i < r.size(); i++) {
                T *featureTmp = new T(img, r[i]);
                p *= oldFeatures[i].prob(*featureTmp);
                delete featureTmp;
            }
            
            p = powl(p, (1.0 / r.size()));
            
            return p;
        }
        
        
        Rect getObjectRect() {
            return objRect;
        }
};







    
} /// End of namespace ot


class LanePF2 {
    public: 
    
    /**
     * 对车道线进行粒子滤波（使用二次曲线拟合）
     * 
     * @return vector<float>    三个参数，分别是二次曲线上的三个点
     */
    template <typename T>
    static vector<Point> poly2(Mat& _iInput) {
        int i;
        
        static int stateNum = 3;
        static int measureNum = 3;
        static int sampleNum = 20000;
        
        
        static CvMat *lowerBound = cvCreateMat(stateNum, 1, CV_32F);
        static CvMat *upperBound = cvCreateMat(stateNum, 1, CV_32F);
        

        Mat iInput(_iInput, Rect(0, 0, _iInput.cols / 2, _iInput.rows));
        
        
        static CvConDensation *con = NULL;
        if (con == NULL) {
            /// 初始化粒子滤波器 
            con = cvCreateConDensation(stateNum, measureNum, sampleNum);
            
            cvmSet(lowerBound, 0, 0, 0.0);
            cvmSet(lowerBound, 1, 0, 0.0);
            cvmSet(lowerBound, 2, 0, 0.0);
            
            cvmSet(upperBound, 0, 0, iInput.cols - 1);
            cvmSet(upperBound, 1, 0, iInput.cols - 1);
            cvmSet(upperBound, 2, 0, iInput.cols - 1);
            float A[3][3] = {
                1, 0, 0,
                0, 1, 0,
                0, 0, 1
            };
            memcpy(con->DynamMatr, A, sizeof(A));
            
            
            cvConDensInitSampleSet(con, lowerBound, upperBound);
            
            CvRNG rng_state = cvRNG(time(NULL));
            for(int i = 0; i < sampleNum; i++){
                con->flSamples[i][0] = float(cvRandInt( &rng_state ) % iInput.cols);
                con->flSamples[i][1] = float(cvRandInt( &rng_state ) % iInput.rows);
                con->flSamples[i][2] = float(cvRandInt( &rng_state ) % iInput.rows);
            }
            
        }
        
        
        /// 计算概率（垂直像素最多法，使用二次曲线拟合：x = a * y^2 + b * y + c）
        
        for (i = 0; i < sampleNum; i++) {
            float a, b, c;
            float x1, x2, x3;
            float sum, p;
            int x, y;
            
            
            p = 0; sum = 0;
            x1 = con->State[0];
            x2 = con->State[1];
            x3 = con->State[2];
            
            vector<float> args = points2Poly2(Point(x1, 0), Point(x2, iInput.rows / 2), Point(x3, iInput.rows - 1));
            a = args[0];
            b = args[1];
            c = args[2];
            
            
            for (y = 0; y < iInput.rows; y++) {
                x = round(a * y * y + b * y + c);
                if (x >= iInput.cols) {
                    x = iInput.cols - 1;
                }
                if (x <= 0) {
                    x = 0;
                }
                sum += iInput.ptr<T>(y)[x];
            }
            //sum /= 255.0;
            
            p = exp(-0.1 * (iInput.rows - sum));
            con->flConfidence[i] = p;
            //cout<<"x1="<<con->flSamples[i][0]<<", x2="<<con->flSamples[i][1]<<", P="<<sum<<endl;
        }
        
        cvConDensUpdateByTime(con);
        
        
        
        
        /// 画出粒子位置
        /*
        for (i = 0; i < sampleNum; i++) {
            Point p1(con->flSamples[i][0], 0);
            Point p2(con->flSamples[i][1], iInput.rows / 2);
            Point p3(con->flSamples[i][2], iInput.rows - 1);
            
            line(iInput, p1, p2, CV_RGB(128, 128, 128), 1);
            line(iInput, p2, p3, CV_RGB(128, 128, 128), 1);
        }
        */
        
        


        /// 设置返回值
        vector<Point> ret;
        
        ret.push_back(Point(con->State[0], 0));
        ret.push_back(Point(con->State[1], iInput.rows / 2));
        ret.push_back(Point(con->State[2], iInput.rows - 1));
        
        printf("Fitted points: (%d,%d), (%d, %d), (%d, %d)\n", ret[0].x, ret[0].y, ret[1].x, ret[1].y, ret[2].x, ret[2].y); 
        
        
        /// 画出预测线条
        //line(_iInput, Point(con->State[0], 0), Point(con->State[1], iInput.rows), CV_RGB(128, 128, 128), 1);
        //cout<<"预测位置："<<con->State[0]<<","<<con->State[1]<<","<<con->State[2]<<endl;

        
        vector<float> args = points2Poly2(ret[0], ret[1], ret[2]);
        Point p0, p1;
        for (int y = 0; y < iInput.rows; y++) {
            int x = round(args[0] * y * y + args[1] * y + args[2]);
            
            if (y == 0) {
                p0 = Point(x, y);
            }
            else {
                p0 = p1;
                p1 = Point(x, y);
            }
            
            line(_iInput, p0, p1, CV_RGB(128, 128, 128), 1);
            
        }
        

        
        return ret;
    }
    
    
    /**
     * 使用三点坐标计算出对应二次曲线的参数
     * x = ay^2 + by + c
     * 
     * @return vector<float>    共三个参数，分别是 a, b, c
     */
    static vector<float> points2Poly2(Point p1, Point p2, Point p3) {
        Mat args(3, 1, CV_32F);
        Mat matY(3, 3, CV_32F);
        Mat matX(3, 1, CV_32F);
        
        matX.ptr<float>(0)[0] = p1.x;
        matX.ptr<float>(1)[0] = p2.x;
        matX.ptr<float>(2)[0] = p3.x;
        
        
        matY.ptr<float>(0)[2] = 1;
        matY.ptr<float>(0)[1] = p1.y;
        matY.ptr<float>(0)[0] = p1.y * p1.y;
        
        matY.ptr<float>(1)[2] = 1;
        matY.ptr<float>(1)[1] = p2.y;
        matY.ptr<float>(1)[0] = p2.y * p2.y;
        
        matY.ptr<float>(2)[2] = 1;
        matY.ptr<float>(2)[1] = p3.y;
        matY.ptr<float>(2)[0] = p3.y * p3.y;
        
        
        /**
        cout<<matX<<endl;
        cout<<matY<<endl;
        cout<<matY.inv()<<endl;
        */
        
        args = matY.inv() * matX;
        
        
        vector<float> ret;
        
        ret.push_back(args.ptr<float>(0)[0]);
        ret.push_back(args.ptr<float>(1)[0]);
        ret.push_back(args.ptr<float>(2)[0]);
        
        return ret;
    }

};








/**
 * 对车道线进行粒子滤波
 * 
 * @return vector<Point>    返回三个点二次拟合点的位置
 */
template <typename T>
vector<Point> lanePF(Mat& _iInput) {
    int i;
    
    static int stateNum = 3;
    static int measureNum = 3;
    static int sampleNum = 2000;
    
    
    static CvMat *lowerBound = cvCreateMat(stateNum, 1, CV_32F);
    static CvMat *upperBound = cvCreateMat(stateNum, 1, CV_32F);

    Mat iInput(_iInput, Rect(0, 0, _iInput.cols / 2, _iInput.rows));
    
    
    static CvConDensation *con = NULL;
    if (con == NULL) {
        /// 初始化粒子滤波器 
        con = cvCreateConDensation(stateNum, measureNum, sampleNum);
        
        cvmSet(lowerBound, 0, 0, 0.0);
        cvmSet(lowerBound, 1, 0, 0.0);
        cvmSet(lowerBound, 2, 0, 0.0);
        
        cvmSet(upperBound, 0, 0, iInput.cols / 2);
        cvmSet(upperBound, 1, 0, iInput.cols / 2);
        cvmSet(upperBound, 2, 0, iInput.cols / 2);
        float A[3][3] = {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1
        };
        memcpy(con->DynamMatr, A, sizeof(A));
        
        
        cvConDensInitSampleSet(con, lowerBound, upperBound);
        
        CvRNG rng_state = cvRNG(time(NULL));
        for(int i=0; i < sampleNum; i++){
            con->flSamples[i][0] = float(cvRandInt( &rng_state ) % iInput.cols); //width
            con->flSamples[i][1] = float(cvRandInt( &rng_state ) % iInput.cols);//height
            con->flSamples[i][2] = float(cvRandInt( &rng_state ) % iInput.cols);//height
        }
        
    }
    

    
    /// 计算概率（垂直像素最多法）
    
    for (i = 0; i < sampleNum; i++) {
        int x, y;
        float e, p;
        
        p = 0; e = 0;
        
        for (y = 0; y < iInput.rows / 2; y++) {
            x = con->flSamples[i][0] + (con->flSamples[i][1] - con->flSamples[i][0]) * (1.0 * y / (iInput.rows / 2));
            e += iInput.ptr<T>(y)[x];
        }
        
        for (y = iInput.rows / 2; y < iInput.rows; y++) {
            x = con->flSamples[i][1] + (con->flSamples[i][2] - con->flSamples[i][1]) * (1.0 * (y - iInput.rows / 2) / (iInput.rows - iInput.rows / 2));
            e += iInput.ptr<T>(y)[x];
        }
        
        
        
        e /= 255.0;
        
        /// 我们期望的车道线是直的，在此减小弯曲车道线的概率
        e -= abs((con->flSamples[i][0] + con->flSamples[i][2]) / 2 - con->flSamples[i][1]) * 1;
        
        
        
        p = exp(-0.01 * (iInput.rows - e));
        con->flConfidence[i] = p;
        //cout<<"x1="<<con->flSamples[i][0]<<", x2="<<con->flSamples[i][1]<<", P="<<p<<endl;
    }
    cout<<"更新前："<<con->State[0]<<endl;
    cvConDensUpdateByTime(con);
    
    
    
    /*
    for (i = 0; i < sampleNum; i++) {
        Point p1(con->flSamples[i][0], con->flSamples[i][1]);
        Point p2(p1.x + totalLen, p1.y);
        
        line(iInput, p1, p2, CV_RGB(128, 128, 128), 1);
    }
    */
    Point p1, p2, p3;
    p1 = Point(con->State[0], 0);
    p2 = Point(con->State[1], iInput.rows / 2);
    p3 = Point(con->State[2], iInput.rows);
    line(_iInput, p1, p2, CV_RGB(128, 128, 128), 1);
    line(_iInput, p2, p3, CV_RGB(128, 128, 128), 1);
    cout<<"预测位置："<<con->State[0]<<endl;


    /**
     * 用二次曲线将三个点拟合起来
     */
    /*
    vector<float> args = points2Poly2(p1, p2, p3);
    printf("a=%0.1f, b=%0.1f, c=%0.1f\n", args[0], args[1], args[2]);
    for (int y = 0; y < iInput.rows; y++) {
        int x = round(args[0] * y * y + args[1] * y + args[2]);
        if (x >= iInput.cols) {
            x = iInput.cols - 1;
        }
        if (x < 0) {
            x = 0;
        }
        circle(iInput, Point(x, y), 2, CV_RGB(128, 128, 128), 1);
    }
    */
    
    vector<Point> ret;
    
    ret.push_back(p1);
    ret.push_back(p2);
    ret.push_back(p3);
    
    return ret;
}



/**
 * 返回当前 Unix 时间戳，精确到微秒
 */
double getMicroTime() {
    struct timeval tv;
    double ret;
    
    gettimeofday(&tv, NULL);
    
    ret = 1.0 * tv.tv_sec + tv.tv_usec / 1000000.0;
        
    return ret;
}
