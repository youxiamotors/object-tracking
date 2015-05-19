#ifndef __FEATURE_MEANFUNC_HPP__
#define __FEATURE_MEANFUNC_HPP__ 1

#include "feature_base.hpp"

namespace ot {
/**
 * 使用可比较哈希值计算图像相似度
 */
class MeanFunc : public FeatureBase {
    protected:
        Size size;
        static const int sizeWidth = 20;
        static const int sizeHeight = 20;
        unsigned char* hash;
        double p;
        
    public:
        MeanFunc() {
            size = Size(sizeWidth, sizeHeight);
        };
        ~MeanFunc() {
            if (hash != NULL) {
                free(hash);
                hash = NULL;
            }
        }
        
        MeanFunc(Mat& _input, Rect roi) {
            size = Size(sizeWidth, sizeHeight);
            hash = (unsigned char*)malloc(size.width * size.height);
            
            fixROI(roi, _input);
            
            Mat iROI = Mat(_input, roi);
            calc(iROI);
        }
        

        void calc(Mat& _input) {   
            Mat iResize;
            
            imshow("_input", _input);
            
            resize(_input, iResize, size);
            
            for (int x = 0; x < iResize.cols; x++) {
                for (int y = 0; y < iResize.rows; y++) {
                    hash[y + x * y] = iResize.ptr<unsigned char>(y)[x];
                }
            }
        }
        
        double prob(MeanFunc& feature) {
            /// 计算汉明距离
            assert(this->size == feature.size);
            
            int diffs = 0;
            
            for (long i = 0; i < size.width * size.height; i++) {
                unsigned char c = hash[i] ^ feature.hash[i];
                
                for (int k = 0; k < 8; k++) {
                    if ((c & (unsigned char)0x01) == (unsigned char)0x01) {
                        diffs++;
                    }
                }
            }
            
            
            return diffs  / (1.0 * size.width * size.height);
        }
};

}
#endif
