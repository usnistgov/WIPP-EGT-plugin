//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGERULE_H
#define NEWEGT_MERGERULE_H

#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include <egt/utils/pyramid/Pyramid.h>
#include <egt/FeatureCollection/Data/ViewAnalyzeBlock.h>

namespace egt {

class MergeRule : public htgs::IRule<ViewAnalyse, ViewAnalyseBlock> {

    public:

        MergeRule(pb::Pyramid &pyramid) : pyramid(pyramid) {

            levels = std::vector<std::map<std::pair<uint32_t, uint32_t>, ViewAnalyseBlock*>>(pyramid.getNumLevel());

            for (auto level = 0; level < pyramid.getNumLevel(); level++){
                levels[level] = std::map<std::pair<uint32_t, uint32_t>, ViewAnalyseBlock*>();
            }

        }

        void applyRule(std::shared_ptr<ViewAnalyse> data, size_t pipelineId) override {

            uint32_t blockRow = data->getRow() / branchFactor;
            uint32_t blockCol = data->getCol() / branchFactor;
            uint32_t level = data->getLevel();

            //insert Vanalyse in its corresponding block
            if ( levels[level].find({blockRow, blockCol}) == levels[level].end() ) {
                auto newBlock = new ViewAnalyseBlock(blockRow,blockCol,level);
                levels[level].insert( {{blockRow, blockCol}, newBlock} );
            }

            ViewAnalyseBlock* block = levels[level][{blockRow, blockCol}];
            block->getViewAnalyses().insert( {{data->getRow(), data->getCol()},data} );

            //generated all  levels. We are done.
            if(data->getLevel() == pyramid.getNumLevel() - 1) {
                done = true;
                VLOG(3) << "done generating last level." << std::endl;
                return;
            }

            auto splitCol = ((pyramid.getNumTileCol(level) % branchFactor) != 0);
            auto splitRow = ((pyramid.getNumTileRow(level) % branchFactor) != 0);
            auto isRightMostBlock = (blockCol == (pyramid.getNumTileCol(level) / branchFactor)) ;
            auto isBottomBlock = (blockRow == (pyramid.getNumTileRow(level) / branchFactor)) ;


            //bottom-right block special case
            if( splitCol && splitRow && isRightMostBlock && isBottomBlock ) {
                VLOG(3) << "level " << level + 1 << ": (" << blockRow << ", " << blockCol <<  ")> " << "Bottom right block special case." << std::endl;
                this->addResult(block);
            }
            //right or bottom blocks special cases
            else if ((splitCol && isRightMostBlock)) {
                if(levels[level][{blockRow, blockCol}]->getViewAnalyses().size() == 2) {
                    VLOG(3) << "level " << level + 1 << ": (" << blockRow << ", " << blockCol <<  "). " <<"Edge right block special case." << std::endl;
                    this->addResult(block);
                }
            }
            else if(splitRow && isBottomBlock) {
                if(levels[level][{blockRow, blockCol}]->getViewAnalyses().size() == 2) {
                    VLOG(3) << "level " << level + 1 << ": (" << blockRow << ", " << blockCol <<  "). " <<"Edge bottom block special case." << std::endl;
                    this->addResult(block);
                }
            }
            //regular case block
            else {
                if(levels[level][{blockRow, blockCol}]->getViewAnalyses().size() == 4) {
                    VLOG(3) << "level " << level + 1 << ": (" << blockRow << ", " << blockCol <<  "). " << "Regular block." << std::endl;
                    this->addResult(block);
                }
            }


        }


        bool canTerminateRule(size_t pipelineId) override {
            return done;
        }

        bool done = false;
        std::vector<std::map<std::pair<uint32_t, uint32_t>, ViewAnalyseBlock*>> levels;
        pb::Pyramid pyramid;
        uint32_t branchFactor = 2;
    };




}

#endif //NEWEGT_MERGERULE_H
