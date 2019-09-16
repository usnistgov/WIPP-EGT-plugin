//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_VIEWANALYZEBLOCK_H
#define NEWEGT_VIEWANALYZEBLOCK_H


#include <htgs/api/IData.hpp>
#include <vector>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>

namespace egt {

    class ViewAnalyseBlock : public htgs::IData {


    public:
        ViewAnalyseBlock() = default;

        std::vector<std::shared_ptr < ViewAnalyse>> viewAnalyses{};


    };
}
#endif //NEWEGT_VIEWANALYZEBLOCK_H
