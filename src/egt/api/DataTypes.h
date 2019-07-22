//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-09.
//

#ifndef EGT_DATATYPES_H
#define EGT_DATATYPES_H
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>
#include <opencv/cv.hpp>

enum class ImageDepth {
    _8U,
    _16U,
    _32F
};


int convertToOpencvType(ImageDepth depth){
    switch (depth) {
        case ImageDepth::_8U:
            return CV_8U;
        case ImageDepth::_16U:
            return CV_16U;
        case ImageDepth ::_32F:
            return CV_32F;
        default:
            return CV_16U;
    }
}


int calculateBitsPerSample(ImageDepth depth){
    switch (depth) {
        case ImageDepth::_8U:
            return 1;
        case ImageDepth::_16U:
            return 2;
        case ImageDepth ::_32F:
            return 4;
        default:
            return 2;
    }
}

#endif //EGT_DATATYPES_H
