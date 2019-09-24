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

        enum BLOCK_TYPE {
            REGULAR4, VERTICAL2, HORIZONTAL2, SINGLE1
        };

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

        BLOCK_TYPE getType() const {
            return type;
        }

        void setType(BLOCK_TYPE type) {
            ViewAnalyseBlock::type = type;
        }

        std::ostringstream printBlock(){

            std::ostringstream oss;

            oss << "block - level" << level <<": (" << row << ", " << col << "). ";

            for(auto v : viewAnalyses){
                oss <<  "(" << v.first.first << "," << v.first.second << ") ; ";
            }

            return oss;

        }

    private:

        std::map<std::pair<uint32_t,uint32_t >, std::shared_ptr < ViewAnalyse>> viewAnalyses{};

        uint32_t row;

        uint32_t col;

        uint32_t level;

        BLOCK_TYPE type;


    };
}
#endif //NEWEGT_VIEWANALYZEBLOCK_H
