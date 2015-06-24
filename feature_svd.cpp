#include <stdio.h>
#include <iostream>
#include "feature_svd.hpp"

using namespace std;

namespace ot {
FeatureSVD::FeatureSVD() {
    size = Size(sizeWidth, sizeHeight);
}
    
    
FeatureSVD::~FeatureSVD() {

}

FeatureSVD::FeatureSVD(Mat& _input, Rect _roi) {
    size = Size(sizeWidth, sizeHeight);
    
    Rect roi = _roi;
    
    fixROI(roi, _input);
    
    Mat iROI = Mat(_input, roi);
    calc(iROI);
}


void FeatureSVD::calc(Mat& _input) {   
    Mat iResize, iGray, iGray32;
    
    resize(_input, iResize, size);
    cvtColor(iResize, iGray, CV_RGB2GRAY);
    iGray.convertTo(iGray32, CV_32F);
    
    SVD svd(iGray32, SVD::NO_UV);
    s = svd.w.clone();
}

double FeatureSVD::prob(FeatureSVD& feature) {
    return probAngle(feature);
}

double FeatureSVD::probDist(FeatureSVD& feature) {
    /// 计算 SVD 距离
    double dist = 0;
    double p;
    
    assert(s.size() == feature.s.size());
    
    for (int i = 0; i < s.rows; i++) {
        double sum;
        
        sum = s.at<float>(i) - feature.s.at<float>(i);
        sum *= sum;
        dist += sum;
    }
    
    dist = sqrt(dist);
    
    p = exp(-0.001 * dist);
    
    return p;
}



double FeatureSVD::probAngle(FeatureSVD& feature) {
    /// 计算 SVD 角度
    double p;
    double xysum = 0;
    double xsum = 0, ysum = 0;
    
    for (int i = 0; i < s.rows; i++) {
        float x, y;
        
        x = s.ptr<float>(i)[0];
        y = feature.s.ptr<float>(i)[0];
        
        xysum += x * y;
        xsum += x * x;
        ysum += y * y;
    }
    
    xsum = sqrt(xsum);
    ysum = sqrt(ysum);
    
    p = xysum / (xsum * ysum);
    
    
    return p;
}



}
