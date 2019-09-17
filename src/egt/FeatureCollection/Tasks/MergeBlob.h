//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGEBLOB_H
#define NEWEGT_MERGEBLOB_H

#include <tiffio.h>
#include <htgs/api/ITask.hpp>
#include <cstdint>
#include <utility>
#include <FastImage/FeatureCollection/tools/UnionFind.h>
#include <egt/FeatureCollection/Data/ListBlobs.h>
#include <egt/utils/FeatureExtraction.h>
#include <egt/api/DerivedSegmentationParams.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include "egt/FeatureCollection/Data/ViewAnalyzeBlock.h"

namespace egt {

class MergeBlob : public htgs::ITask<ViewAnalyseBlock, ViewAnalyse> {

public:

    explicit MergeBlob(size_t numThreads) : ITask(numThreads) {}

    void executeTask(std::shared_ptr<ViewAnalyseBlock> data) override {

        result = new ViewAnalyse(data->getRow(), data->getCol(), data->getLevel() + 1);


        auto views = data->getViewAnalyses();

        //For each view, check BOTTOM, RIGHT, BOTTOM-RIGHT and TOP-RIGHT view FOR POTENTIAL MERGES
        //merge within this block are performed
        //others are scheduled for the next level
        for(auto &entry : views) {
            auto coordinates = entry.first;
            auto row = coordinates.first, col = coordinates.second;
            auto view = entry.second;
            if(views.find({row + 1, col}) != views.end()){
                merge(view, views[{row + 1, col}]);
            }
            else {
                addToNextLevelMerge(view , {row + 1, col}, {data->getRow() + 1, data->getCol()});
            }
            if(views.find({row, col + 1}) != views.end()){
                merge(view, views[{row, col + 1}]);
            }
            else {
                addToNextLevelMerge(view , {row , col + 1}, {data->getRow(), data->getCol() + 1});
            }
            if(views.find({row + 1, col + 1}) != views.end()){
                merge(view, views[{row + 1, col + 1}]);
            }
            else {
                addToNextLevelMerge(view , {row + 1 , col + 1}, {data->getRow() + 1, data->getCol() + 1});
            }
            if(views.find({row + 1, col + 1}) != views.end()){
                merge(view, views[{row + 1, col + 1}]);
            }
            else {
                addToNextLevelMerge(view , {row + 1 , col + 1}, {data->getRow() - 1, data->getCol() - 1});
            }
        }

        VLOG(3) << "Merge produce ViewAnalyse for level " << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). ";
        this->addResult(result);
    }

    ITask<ViewAnalyseBlock, ViewAnalyse> *copy() override {
        return new MergeBlob(this->getNumThreads());
    }

    /**
     * Merge blobs at the border of two contiguous tiles.
     */
    void merge(std::shared_ptr<ViewAnalyse> v1, std::shared_ptr<ViewAnalyse> v2){
        VLOG(3) << "merge...";
        for(auto entry : v1->getToMerge()) {
            auto coordinates = entry.first;
            auto candidates = v2->getToMerge()[coordinates];
            //amongst candidates, find the correct blob and merge
            //when we are done all blobs are merged on this side
        }
    }

    /**
     * Record merges that needs to happen at the next level
     */
    void addToNextLevelMerge(std::shared_ptr<ViewAnalyse> v1, std::pair<uint32_t,uint32_t> coordinates, std::pair<uint32_t,uint32_t> contiguousBlockCoordinates){
        //merge all entries of v1->getToMerge()[coordinates] into result->getToMerge()[contiguousBlockCoordinates]
        //we will merge them at the next level
        //empty if we are at the border
    }

    //Managed by HTGS, no need to delete
    ViewAnalyse* result{};


};

}

#endif //NEWEGT_MERGEBLOB_H
