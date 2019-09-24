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

    explicit MergeBlob(size_t numThreads, pb::Pyramid &pyramid) : ITask(numThreads) , pyramid(pyramid) {}

    void executeTask(std::shared_ptr<ViewAnalyseBlock> data) override {

        result = new ViewAnalyse(data->getRow(), data->getCol(), data->getLevel() + 1);


        VLOG(3) << "Merging block : " << data->printBlock().str();

        auto views = data->getViewAnalyses();

        //first copy all the blob groups we will need to work with
        for(auto &entry : views) {
            auto view = entry.second;
            for(auto parentSon : view->getBlobsParentSons()) {
                result->getBlobsParentSons().insert(parentSon);
            }
        }

        switch (data->getType()){
            case ViewAnalyseBlock::REGULAR4: {

                //TOP LEFT CORNER
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;
                merge(view, views[{row + 1, col}]); //BOTTOM
                merge(view, views[{row, col + 1}]); //RIGHT
                merge(view, views[{row + 1, col + 1}]); //BOTTOM-RIGHT

                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT

//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
//                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

                //TOP RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                merge(view, views[{row + 1, col}]); //BOTTOM
                merge(view, views[{row + 1, col - 1}]); //BOTTOM-LEFT

                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-LEFT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                merge(view, views[{row, col + 1}]); // RIGHT

                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
//                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT
                break;
            }
            case ViewAnalyseBlock::VERTICAL2: {

                //TOP
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;

                merge(view, views[{row + 1, col}]); //BOTTOM

                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                break;
            }
            case ViewAnalyseBlock::HORIZONTAL2: {
                //LEFT
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;

                merge(view, views[{row, col + 1}]); //RIGHT

                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); // LEFT
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT


                //RIGHT
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                break;
            }
            case ViewAnalyseBlock::SINGLE1: {
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;
                addToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
            }
        }

        VLOG(3) << "Merge produce ViewAnalyse for level " << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). ";

        auto mergeCount= 0;
        for(auto blobCoordPair : result->getToMerge()) {
            VLOG(3) << "(" << blobCoordPair.first.first << "," << blobCoordPair.first.second << "). size : " << blobCoordPair.second.size();
        }

        this->addResult(result);
    }

    ITask<ViewAnalyseBlock, ViewAnalyse> *copy() override {
        return new MergeBlob(this->getNumThreads(), this->pyramid);
    }

    /**
     * Merge blobs at the border of two contiguous tiles.
     */
    void merge(std::shared_ptr<ViewAnalyse> v1, std::shared_ptr<ViewAnalyse> v2){
        VLOG(3) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "merging tile at level " << result->getLevel() - 1 << " : " << "(" << v1->getRow() << "," << v1->getCol() << ") & (" << v2->getRow() << "," << v2->getCol() << ")"  << " ...";

        //find corresponding merge regions in each view
        auto blobsToMerge = v1->getToMerge()[{v2->getRow(),v2->getCol()}];
        auto otherBlobsToMerge = v2->getToMerge()[{v1->getRow(),v1->getCol()}];

        if(blobsToMerge.empty()) {
            VLOG(5) << "Nothing to do in merge region for tiles : "<< "(" << v1->getRow() << "," << v1->getCol() << ") & " << "(" << v2->getRow() << ")," << v2->getCol();
            return;
        }

        uint32_t mergeCount1 = 0, mergeCount2 = 0;

        //TODO REMOVE. THIS IS ONLY FOR DEBUGGING
        for (auto &blobPixelPair : blobsToMerge) {
            for (auto coordinates : blobPixelPair.second) {
                mergeCount1++;
            }
        }
        for (auto &blobPixelPair : otherBlobsToMerge) {
            for (auto coordinates : blobPixelPair.second) {
                mergeCount2++;
            }
        }
        assert(mergeCount1 == mergeCount2);

        //for each blob's border pixel to merge, find the corresponding blobs to merge in the adjacent tile
        for(auto &blobPixelPair : blobsToMerge) {
                auto blob = blobPixelPair.first;
                for(auto coordinates : blobPixelPair.second) {
                        for(auto otherBlobPixelPair : otherBlobsToMerge) {
                            auto otherBlob = otherBlobPixelPair.first;
                            if (otherBlobPixelPair.first->isPixelinFeature(coordinates.first, coordinates.second)) {
                                //merge the blob groups
                                VLOG(5) << "Creating merged blob from blobsToMerge : blob_" << blob->getTag() << ", blob_" << otherBlob->getTag();
                                UnionFind<Blob> uf{};
                                blob->decreaseMergeCount();
                                otherBlob->decreaseMergeCount();
                                auto root1 = uf.find(blob);
                                auto root2 = uf.find(otherBlob);

                                auto blobGroup1 = result->getBlobsParentSons()[root1];

                                // blobs already connected through another path
                                if (root1 == root2) {
                                    VLOG(5) << "blob_" << blob->getTag() << "& blob_" << otherBlob->getTag()
                                            << " are already connected through another path. Common root is : blob_"
                                            << root1->getTag();

                                    //are we done merging the whole blob?
                                    auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                          [](uint32_t i, const Blob *b) {
                                                                              return b->getMergeCount() + i;
                                                                          });

                                    //if yes, record the blobgroup as final
                                    if (mergeLeftCount == 0) {
                                        result->getFinalBlobsParentSons()[root1].insert(blobGroup1.begin(), blobGroup1.end());
                                        result->getBlobsParentSons().erase(root1);
                                    }
                                }
                                // we build a blob group from connected blob groups
                                else {
                                    auto root = uf.unionElements(blob, otherBlob); //pick one root as the new root
                                    auto blobGroup2 = result->getBlobsParentSons()[root2];
                                    blobGroup1.insert(blobGroup2.begin(), blobGroup2.end()); //merge sons


                                    //are we done merging the whole blob?
                                    auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                          [](uint32_t i, const Blob *b) {
                                                                              return b->getMergeCount() + i;
                                                                          });

                                    //if yes, record the blobgroup as final
                                    if (mergeLeftCount == 0) {
                                        result->getFinalBlobsParentSons()[root].insert(blobGroup1.begin(),
                                                                                       blobGroup1.end());
                                        result->getBlobsParentSons().erase(root1);
                                        result->getBlobsParentSons().erase(root2);
                                    //otherwise keep track of the merged blobgroup
                                    } else {
                                        result->getBlobsParentSons()[root].insert(blobGroup1.begin(), blobGroup1.end());
                                        if (root == root2) {
                                            result->getBlobsParentSons().erase(root1);
                                        } else {
                                            result->getBlobsParentSons().erase(root2);
                                        }
                                    }
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
    void addToNextLevelMerge(std::shared_ptr<ViewAnalyse> currentView, std::pair<uint32_t,uint32_t> tileCoordinates, std::pair<uint32_t,uint32_t> contiguousBlockCoordinates) {
        auto blobsToMerge = currentView->getToMerge()[tileCoordinates];

        auto level = currentView->getLevel();
        if((tileCoordinates.first >= pyramid.getNumTileRow(level) || tileCoordinates.second >= pyramid.getNumTileCol(level)) ||
                   contiguousBlockCoordinates.first >= pyramid.getNumTileRow(level + 1) || contiguousBlockCoordinates.second >= pyramid.getNumTileCol(level + 1))
        {
            return;
        }

        if(!(blobsToMerge).empty()){
            VLOG(5) << "level: " << result->getLevel() << " - block : (" << result->getRow() << ", " << result->getCol() <<  "). "  << " - add merge with block : " << "(" << contiguousBlockCoordinates.first << "," << contiguousBlockCoordinates.second << ")" << "- original tile : " << "(" << tileCoordinates.first << "," << tileCoordinates.second << ")" << "need to merge " << blobsToMerge.size() << " at the next level";
            for(auto blobToPixelEntry : currentView->getToMerge()[tileCoordinates]) {
                result->getToMerge()[contiguousBlockCoordinates].insert(blobToPixelEntry);
            }
        }
    }

    //Managed by HTGS, no need to delete
    ViewAnalyse* result{};

    pb::Pyramid pyramid;

};

}

#endif //NEWEGT_MERGEBLOB_H
