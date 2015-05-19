#ifndef __FEATURE_MEANFUNC_HPP__
#define __FEATURE_MEANFUNC_HPP__ 1

#include <iostream>

#include "feature_base.hpp"

using namespace std;

namespace ot {
/**
 * 使用可比较哈希值计算图像相似度
 */
class MeanFunc : public FeatureBase {
    protected:
        Size size;
        static const int sizeWidth = 20;
        static const int sizeHeight = 20;
        unsigned char* hash[3];
        double p;
        
    public:
        MeanFunc();
        MeanFunc(Mat& _input, Rect roi);
        ~MeanFunc();
        
        void calc(Mat& _input);
        double prob(MeanFunc& feature);
};

}
#endif
