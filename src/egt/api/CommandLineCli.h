//
// Created by gerardin on 7/17/19.
//

#ifndef NEWEGT_COMMANDLINECLI_H
#define NEWEGT_COMMANDLINECLI_H


#include "DataTypes.h"

namespace egt {

    ImageDepth parseImageDepth(const std::string &depth) {
        if (depth == "16U") {
            return ImageDepth::_16U;
        } else if (depth == "8U") {
            return ImageDepth::_8U;
        } else if (depth == "32F") {
                return ImageDepth::_32F;
        } else {
            throw std::invalid_argument("image depth not recognized. Should  be one of : 8U, 16U, 32F");
        }
    }

}

#endif //NEWEGT_COMMANDLINECLI_H
