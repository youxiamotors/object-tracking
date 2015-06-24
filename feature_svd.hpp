#ifndef __FEATURE_FEATURESVD_HPP__
#define __FEATURE_FEATURESVD_HPP__ 1

#include <iostream>

#include "feature_base.hpp"

using namespace std;

namespace ot {
/**
 * 使用奇异值分解计算图像相似度
 */
class FeatureSVD : public FeatureBase {
    protected:
        Size size;
        static const int sizeWidth = 24;
        static const int sizeHeight = 24;
        Mat s;  /// 奇异值
        
    public:
        FeatureSVD();
        FeatureSVD(Mat& _input, Rect roi);
        ~FeatureSVD();
        
        void calc(Mat& _input);
        
        double prob(FeatureSVD& feature);
        double probDist(FeatureSVD& feature);
        double probAngle(FeatureSVD& feature);
};

}
#endif
