#include <iostream>
#include "feature_meanfunc.hpp"

using namespace std;

namespace ot {
MeanFunc::MeanFunc() {
    size = Size(sizeWidth, sizeHeight);
    
    for (int i = 0; i < 3; i++) {
        this->hash[i] = NULL;
    }
}
    
    
MeanFunc::~MeanFunc() {
    for (int i = 0; i < 3; i++) {
        if (this->hash[i] != NULL) {
            free(this->hash[i]);
            this->hash[i] = NULL;
        }
    }
}

MeanFunc::MeanFunc(Mat& _input, Rect _roi) {
    size = Size(sizeWidth, sizeHeight);
    for (int i = 0; i < 3; i++) {
        hash[i] = (unsigned char*)malloc(size.width * size.height);
    }
    
    Rect roi = _roi;
    
    fixROI(roi, _input);
    
    Mat iROI = Mat(_input, roi);
    calc(iROI);
}


void MeanFunc::calc(Mat& _input) {   
    Mat iResize, iGray;
    Mat rgb[3];
    
    resize(_input, iResize, size);
    split(iResize, rgb);
    //cvtColor(iResize, iGray, CV_RGB2GRAY);
    
    for (int i = 0; i < 3; i++) {
        Mat iChannel = rgb[i];
        
        for (int x = 0; x < iChannel.cols; x++) {
            for (int y = 0; y < iChannel.rows; y++) {
                hash[i][y + x * iChannel.rows] = iChannel.ptr<unsigned char>(y)[x];
            }
        }
    }
}

double MeanFunc::prob(MeanFunc& feature) {
    double p;
    
    /// 计算汉明距离，汉明距离越大概率越小
    assert(this->size == feature.size);
    
    int diffs[3] = {0};
    
    for (int idx = 0; idx < 3; idx++) {
        for (long i = 0; i < size.width * size.height; i++) {
            unsigned char c = hash[idx][i] ^ feature.hash[idx][i];
            
            for (int k = 0; k < 8; k++) {
                if ((c & (unsigned char)0x01) == (unsigned char)0x01) {
                    diffs[idx]++;
                }
                
                c >>= 1;
            }
        }
    }
    
    
    /*
    p = diffs  / (8.0 * size.width * size.height);
    p = 1 - p;
    p *= p;
    */
    
    //p = exp(-0.01 * diffs);
    
    double prob[3] = {0};
    for (int i = 0; i < 3; i++) {
        prob[i] = diffs[i] / (8.0 * size.width * size.height);
        prob[i] = 1 - prob[i];
        prob[i] *= prob[i];
    }
    
    p = pow(prob[0] * prob[1] * prob[2], 1.0 / 3.0);
    //p = (prob[0] + prob[1] + prob[2]) / 3.0;
    
    return p;
}

}
