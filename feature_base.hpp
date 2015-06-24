#ifndef __FEATURE_BASE_HPP__
#define __FEATURE_BASE_HPP__ 1

#include <opencv2/opencv.hpp>

using namespace cv;


namespace ot {
/**
 * 特征类基类
 */
class FeatureBase {
    public:
    
    /**
     * 将一个 ROI 调整为不超过图像边界
     */
    static Rect fixROI(Rect& roi, const Mat& _input) {
        if (roi.x + roi.width > _input.cols || roi.width <= 0) {
            roi.width = 1;
        }
        if (roi.y + roi.height > _input.rows || roi.height <= 0) {
            roi.height = 1;
        }
        if (roi.x >= _input.cols) {
            roi.x = _input.cols - 1;
        }
        else if (roi.x < 0) {
            roi.x = 0;
        }
        if (roi.y >= _input.rows) {
            roi.y = _input.rows - 1;
        }
        else if (roi.y < 0) {
            roi.y = 0;
        }
        
        return roi;
    }
};

}


#endif
