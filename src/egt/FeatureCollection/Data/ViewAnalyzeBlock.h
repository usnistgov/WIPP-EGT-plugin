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

        ViewAnalyseBlock(uint32_t row, uint32_t col, uint32_t level) : row(row), col(col), level(level){}


        std::map<std::pair<uint32_t,uint32_t >, std::shared_ptr < ViewAnalyse>> &getViewAnalyses() {
            return viewAnalyses;
        }

        uint32_t getRow() const {
            return row;
        }

        uint32_t getCol() const {
            return col;
        }

        uint32_t getLevel() const {
            return level;
        }

    private:

        std::map<std::pair<uint32_t,uint32_t >, std::shared_ptr < ViewAnalyse>> viewAnalyses{};

        uint32_t row;

        uint32_t col;

        uint32_t level;


    };
}
#endif //NEWEGT_VIEWANALYZEBLOCK_H
