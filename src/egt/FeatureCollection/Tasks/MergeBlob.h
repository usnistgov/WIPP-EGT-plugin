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

            if(views.find({row - 1, col}) != views.end()){
                merge(view, views[{row - 1, col}]);
            }
            else {
                addToNextLevelMerge(view , {row - 1 , col}, {data->getRow() - 1, data->getCol()});
            }

            if(views.find({row, col - 1}) != views.end()){
                merge(view, views[{row, col - 1}]);
            }
            else {
                addToNextLevelMerge(view , {row , col - 1}, {data->getRow(), data->getCol() - 1});
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
        VLOG(3) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "merging tile : " << "(" << v1->getRow() << "," << v1->getCol() << ") & (" << v2->getRow() << "," << v2->getCol() << ")"  << "merge...";

        auto blobs = v1->getToMerge()[{v2->getRow(),v2->getCol()}];
        auto blobsToMerge = v2->getToMerge()[{v1->getRow(),v1->getCol()}];

        assert (blobs.size() == blobsToMerge.size());

        if(blobs.size() == 0){
            VLOG(3) << "Nothing to merge.";
            return;
        }

        // TODO iterate and remove entry from blobsToMerge as we perform merges
        // also instead of a list of blob, we should maintain UnionFind and merge those as we go.
        // when all mergesa are done, we create the final mask
        for(auto &b : blobs) {
                auto blob = b.first;
                for(auto coordinates : b.second){
                        for(auto other : blobsToMerge) {
                            auto otherBlob = other.first;
                            if (other.first->isPixelinFeature(coordinates.first, coordinates.second)) {
                                createNewBlob(blob,otherBlob);
                            }
                        }
                }
        }
    }

    void createNewBlob(Blob* blob, Blob* otherBlob){
        VLOG(3) << "Creating merged blob from blobs :" << blob->getTag() << "," << otherBlob->getTag();
    }

    /**
     * Record merges that needs to happen at the next level
     */
    void addToNextLevelMerge(std::shared_ptr<ViewAnalyse> v1, std::pair<uint32_t,uint32_t> coordinates, std::pair<uint32_t,uint32_t> contiguousBlockCoordinates){
        auto blobsToMerge = v1->getToMerge()[coordinates];
        if(!(blobsToMerge).empty()){
            VLOG(4) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "block : " << "(" << contiguousBlockCoordinates.first << "," << contiguousBlockCoordinates.second << ")" << "tile : " << "(" << coordinates.first << "," << coordinates.second << ")" << "need to merge " << blobsToMerge.size() << " at the next level";
            for(auto blobToPixelEntry : v1->getToMerge()[coordinates]){
                result->getToMerge()[contiguousBlockCoordinates].insert(blobToPixelEntry);
            }
            //merge all entries of v1->getToMerge()[coordinates] into result->getToMerge()[contiguousBlockCoordinates]
            //we will merge them at the next level
            //empty if we are at the border
        }
        else {
            VLOG(4) << "nothing to merge at  the next level";
        }
    }

    //Managed by HTGS, no need to delete
    ViewAnalyse* result{};

};

}

#endif //NEWEGT_MERGEBLOB_H
