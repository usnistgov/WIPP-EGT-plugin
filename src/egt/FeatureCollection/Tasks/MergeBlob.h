//
// Created by gerardin on 9/16/19.
//

#ifndef NEWEGT_MERGEBLOB_H
#define NEWEGT_MERGEBLOB_H

#include <tiffio.h>
#include <htgs/api/ITask.hpp>
#include <cstdint>
#include <utility>
#include <egt/FeatureCollection/Data/ListBlobs.h>
#include <egt/utils/FeatureExtraction.h>
#include <egt/api/DerivedSegmentationParams.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include "egt/FeatureCollection/Data/ViewAnalyzeBlock.h"
#include <egt/utils/UnionFind.h>

namespace egt {

class MergeBlob : public htgs::ITask<ViewAnalyseBlock, ViewAnalyse> {

public:

    explicit MergeBlob(size_t numThreads) : ITask(numThreads) {}

    void executeTask(std::shared_ptr<ViewAnalyseBlock> data) override {

        result = new ViewAnalyse(data->getRow(), data->getCol(), data->getLevel() + 1);


        auto views = data->getViewAnalyses();

        for(auto &entry : views) {
            auto view = entry.second;
            for(auto parentSon : view->getBlobsParentSons()) {
                result->getBlobsParentSons().insert(parentSon);
            }
        }

        //For each view, check BOTTOM, RIGHT, BOTTOM-RIGHT and TOP-RIGHT view FOR POTENTIAL MERGES
        //merges within this block are performed
        //others are scheduled for the next level
        for(auto &entry : views) {
            auto coordinates = entry.first;
            auto row = coordinates.first, col = coordinates.second;
            auto view = entry.second;

            //BOTTOM TILE
            if(views.find({row + 1, col}) != views.end()){
                merge(view, views[{row + 1, col}]);
            }
            else {
                addToNextLevelMerge(view , {row + 1, col}, {data->getRow() + 1, data->getCol()});
            }

            //RIGHT TILE
            if(views.find({row, col + 1}) != views.end()){
                merge(view, views[{row, col + 1}]);
            }
            else {
                addToNextLevelMerge(view , {row , col + 1}, {data->getRow(), data->getCol() + 1});
            }

            //BOTTOM RIGHT TILE
            if(views.find({row + 1, col + 1}) != views.end()){
                merge(view, views[{row + 1, col + 1}]);
            }
            else {
                addToNextLevelMerge(view , {row + 1 , col + 1}, {data->getRow() + 1, data->getCol() + 1});
            }

            //KEEP TRACK OF CORRESPONDING MERGES ON THE TOP, LEFT AND TOP-LEFT
            addToNextLevelMerge(view , {row - 1 , col}, {data->getRow() - 1, data->getCol()});
            addToNextLevelMerge(view , {row , col - 1}, {data->getRow(), data->getCol() - 1});
            addToNextLevelMerge(view , {row - 1 , col - 1}, {data->getRow() - 1, data->getCol() - 1});
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
        VLOG(3) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "merging tile : " << "(" << v1->getRow() << "," << v1->getCol() << ") & (" << v2->getRow() << "," << v2->getCol() << ")"  << " ...";

        //find corresponding merge regions in each view
        auto blobsToMerge = v1->getToMerge()[{v2->getRow(),v2->getCol()}];
        auto otherBlobsToMerge = v2->getToMerge()[{v1->getRow(),v1->getCol()}];

        assert (blobsToMerge.size() == otherBlobsToMerge.size());

        if(blobsToMerge.empty()){
            VLOG(3) << "Nothing to merge in tile ."<< "(" << v1->getRow() << "," << v1->getCol() << ")";
            return;
        }

        //IMPORTANT : WE REMOVE ALL THE BLOBS MERGED IN THE OTHER TILE SO WE DO NOT CARRY IRRELEVANT MERGES TO THE NEXT LEVEL
        v2->getToMerge().erase({v1->getRow(),v1->getCol()});

        //for each blob to merge, find the corresponding blob in the adjacent tile
        for(auto &blobPixelPair : blobsToMerge) {
                auto blob = blobPixelPair.first;
                for(auto coordinates : blobPixelPair.second) {
                        for(auto otherBlobPixelPair : otherBlobsToMerge) {
                            auto otherBlob = otherBlobPixelPair.first;
                            if (otherBlobPixelPair.first->isPixelinFeature(coordinates.first, coordinates.second)) {
                                //merge the blob groups
                                VLOG(3) << "Creating merged blob from blobsToMerge :" << blob->getTag() << "," << otherBlob->getTag();
                                UnionFind<Blob> uf{};
                                auto root1 = uf.find(blob);
                                auto root2 = uf.find(otherBlob);
                                if(root1 == root2){
                                    //TODO after pixel flattening, this should never happen. For now we just ignore
                                    continue;
                                }
                                auto root = uf.unionElements(blob, otherBlob); //pick one root as the new root
                                auto blobGroup1 = v1->getBlobsParentSons()[root1];
                                auto blobGroup2 = v2->getBlobsParentSons()[root2];
                                blobGroup1.insert(blobGroup2.begin(), blobGroup2.end()); //merge sons
                                result->getBlobsParentSons()[root].insert(blobGroup1.begin(), blobGroup1.end());
                                if(root == root2){
                                    result->getBlobsParentSons().erase(root1);
                                } else {
                                    result->getBlobsParentSons().erase(root2);
                                }
                            }
                        }
                }
        }
    }

      /**
      * Record merges that needs to happen at the next level
      * @param currentView viewAnalyze we need to propagate merges from
      * @param tileCoordinates the coordinates of the merge region
      * @param contiguousBlockCoordinates the coordinates of the new merge region at the next level.
      */
    void addToNextLevelMerge(std::shared_ptr<ViewAnalyse> currentView, std::pair<uint32_t,uint32_t> tileCoordinates, std::pair<uint32_t,uint32_t> contiguousBlockCoordinates){
        auto blobsToMerge = currentView->getToMerge()[tileCoordinates];
        if(!(blobsToMerge).empty()){
            VLOG(4) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "block : " << "(" << contiguousBlockCoordinates.first << "," << contiguousBlockCoordinates.second << ")" << "tile : " << "(" << tileCoordinates.first << "," << tileCoordinates.second << ")" << "need to merge " << blobsToMerge.size() << " at the next level";
            for(auto blobToPixelEntry : currentView->getToMerge()[tileCoordinates]){
                result->getToMerge()[contiguousBlockCoordinates].insert(blobToPixelEntry);
            }
            //merge all entries of currentView->getToMerge()[tileCoordinates] into result->getToMerge()[contiguousBlockCoordinates]
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
