#include <stdio.h>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "feature_hog.hpp"

using namespace std;

namespace ot {
FeatureHOG::FeatureHOG() {
    size = Size(sizeWidth, sizeHeight);
}
    
    
FeatureHOG::~FeatureHOG() {

}

FeatureHOG::FeatureHOG(Mat& _input, Rect _roi) {
    size = Size(sizeWidth, sizeHeight);
    
    Rect roi = _roi;
    
    fixROI(roi, _input);
    
    Mat iROI = Mat(_input, roi);
    calc(iROI);
}


void FeatureHOG::calc(Mat& _input) {   
    Mat iResize, iRGB[3], iGray;
    vector<float> ders;
    
    resize(_input, iResize, size);
    split(iResize, iRGB);
    //cvtColor(iResize, iGray, CV_RGB2GRAY);
    
    
    for (int idx = 0; idx < 3; idx++) {
        iGray = iRGB[idx];
        
        /// 调整第 4 个 size 参数也可以控制直方图 bin 数量
        HOGDescriptor hog(Size(24, 24), Size(24, 24), Size(4, 4), Size(12, 12), 9);
            
        hog.compute(iGray, ders, Size(24, 24), Size(0, 0));
        
        //fprintf(stderr, "size = %lu\n", ders.size());
        
        h[idx] = Mat(1, ders.size(), CV_32F);
        for (size_t i = 0; i < ders.size(); i++) {
            h[idx].ptr<float>(0)[i] = ders[i];
        }
    }
}

double FeatureHOG::prob(FeatureHOG& feature) {
    double p = 1;
    double bc[3];
    
    for (int i = 0; i < 3; i++) {
        bc[i] = compareHist(h[i], feature.h[i], CV_COMP_BHATTACHARYYA);
        p *= exp(-1.0 * bc[i]);
    }
    
    
    return p;
}


}
