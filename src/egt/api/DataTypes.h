//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-09.
//

#ifndef EGT_DATATYPES_H
#define EGT_DATATYPES_H


#include <string>
#include <stdexcept>
#include <opencv2/core/hal/interface.h>

namespace egt {

    enum class ImageDepth {
        _8U,
        _16U,
        _32U,
        _32F
    };


    ImageDepth parseImageDepth(const std::string &depth) {
        if (depth == "16U") {
            return ImageDepth::_16U;
        } else if (depth == "8U") {
            return ImageDepth::_8U;
        } else if (depth == "32U") {
            return ImageDepth ::_32U;
        }
          else if (depth == "32F") {
            return ImageDepth::_32F;
        } else {
            throw std::invalid_argument("image depth not recognized. Should  be one of : 8U, 16U, 32F");
        }
    }

    int convertToOpencvType(ImageDepth depth) {
        switch (depth) {
            case ImageDepth::_8U:
                return CV_8U;
            case ImageDepth::_16U:
                return CV_16U;
            case ImageDepth::_32F:
                return CV_32F;
            default:
                return CV_16U;
        }
    }


    int calculateBitsPerSample(ImageDepth depth) {
        switch (depth) {
            case ImageDepth::_8U:
                return 1;
            case ImageDepth::_16U:
                return 2;
            case ImageDepth::_32F:
                return 4;
            default:
                return 2;
        }
    }

    enum class JoinOperator {
        AND,
        OR
    };

    JoinOperator parseJoinOperator(const std::string &joinOperatorString) {

        if (joinOperatorString == "and") {
            return JoinOperator::AND;
        } else if (joinOperatorString == "or") {
            return JoinOperator::OR;
        } else {
            throw std::invalid_argument("join operator should be one of: 'and','or'");
        }
    };

}

#endif //EGT_DATATYPES_H
