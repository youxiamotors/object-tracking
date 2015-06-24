#ifndef __FEATURE_FEATUREHOG_HPP__
#define __FEATURE_FEATUREHOG_HPP__ 1

#include <iostream>

#include "feature_base.hpp"

using namespace std;

namespace ot {
/**
 * 使用奇异值分解计算图像相似度
 */
class FeatureHOG : public FeatureBase {
    protected:
        Size size;
        static const int sizeWidth = 24;
        static const int sizeHeight = 24;
        Mat h[3];
        
    public:
        FeatureHOG();
        FeatureHOG(Mat& _input, Rect roi);
        ~FeatureHOG();
        
        void calc(Mat& _input);
        
        double prob(FeatureHOG& feature);
};

}
#endif
