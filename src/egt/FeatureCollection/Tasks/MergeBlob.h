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

    explicit MergeBlob(size_t numThreads, pb::Pyramid &pyramid, SegmentationOptions* options) : ITask(numThreads) , pyramid(pyramid), options(options)  {}

    void executeTask(std::shared_ptr<ViewAnalyseBlock> data) override {

        result = new ViewAnalyse(data->getRow(), data->getCol(), data->getLevel() + 1);
      //  tree = new AABBTree<Blob>();

        VLOG(3) << data->printBlock().str() << " - Merging holes...";
        mergeAllHoles(data);
        VLOG(3) << data->printBlock().str() << " - Merging blobs...";
        mergeAllBlobs(data);
        filterHoles();
        filterFinalBlobs();


        VLOG(3) << "ViewAnalyse produced after merging block " << "(" << result->getRow() << ", " << result->getCol() <<  ") " << " at level " << result->getLevel();

   //     delete tree;

        this->addResult(result);
    }


    void buildAABBTree() {

        auto blobs = std::vector<Blob>();

        for(auto parentHole : result->getBlobsParentSons()){
            for(auto blob : parentHole.second) {
                blobs.push_back(*blob);
            }
        }
        for(auto finalParentBlob : result->getFinalBlobsParentSons()){
            for(auto blob : finalParentBlob.second) {
                blobs.push_back(*blob);
            }
        }

        tree->preprocess(&blobs);
    }

    void filterFinalBlobs() {

    }


//    Blob *getBlobFromPixel(uint32_t row, uint32_t col) {
//        for (auto Blob : tree->objectsContain(Vector2<double>((double) col,
//                                                                  (double) row))) {
//            if (Blob->isPixelinFeature(row, col)) {
//                return Blob;
//            }
//        }
//        return nullptr;
//    }



    void filterHoles() {
        result->filterHoles(options->MIN_HOLE_SIZE);
    }


        std::string getName() override {
            return "MergeBlob";
        }


    void mergeAllBlobs(std::shared_ptr<ViewAnalyseBlock> data) {
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
                mergeBlobs(view, views[{row + 1, col}]); //BOTTOM
                mergeBlobs(view, views[{row, col + 1}]); //RIGHT
                mergeBlobs(view, views[{row + 1, col + 1}]); //BOTTOM-RIGHT

                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT

//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
//                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

                //TOP RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                mergeBlobs(view, views[{row + 1, col}]); //BOTTOM
                mergeBlobs(view, views[{row + 1, col - 1}]); //BOTTOM-LEFT

                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-LEFT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                mergeBlobs(view, views[{row, col + 1}]); // RIGHT

                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addBlobsToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

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

                mergeBlobs(view, views[{row + 1, col}]); //BOTTOM

                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT

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

                mergeBlobs(view, views[{row, col + 1}]); //RIGHT

                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); // LEFT
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT


                //RIGHT
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                break;
            }
            case ViewAnalyseBlock::SINGLE1: {
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;
                addBlobsToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addBlobsToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addBlobsToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addBlobsToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addBlobsToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addBlobsToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addBlobsToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
            }
        }
    }

    ITask<ViewAnalyseBlock, ViewAnalyse> *copy() override {
        return new MergeBlob(this->getNumThreads(), this->pyramid, this->options);
    }

    /**
     * Merge blobs at the border of two contiguous tiles.
     */
    void mergeBlobs(std::shared_ptr<ViewAnalyse> v1, std::shared_ptr<ViewAnalyse> v2){
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

        VLOG(2) << "Number of merges to do between tiles : level "<< result->getLevel() - 1 << " - (" << v1->getRow() << "," << v1->getCol() << ") & " << "(" << v2->getRow() << "," << v2->getCol() << ") : " << mergeCount1;

        //for each blob's border pixel to merge, find the corresponding blobs to merge in the adjacent tile
        for(auto &blobPixelPair : blobsToMerge) {
                auto blob = blobPixelPair.first;
                for(auto coordinates : blobPixelPair.second) {
                        for(auto otherBlobPixelPair : otherBlobsToMerge) {
                            auto otherBlob = otherBlobPixelPair.first;
                            if (otherBlobPixelPair.first->isPixelinFeature(coordinates.first, coordinates.second)) {
                                //merge the blob groups
                                VLOG(5) << "Creating merged blob from blobsToMerge : blob_" << blob->getId() << ", blob_" << otherBlob->getId();
                                UnionFind<Blob> uf{};
                                blob->decreaseMergeCount();
                                otherBlob->decreaseMergeCount();
                                auto root1 = uf.find(blob);
                                auto root2 = uf.find(otherBlob);

                                auto blobGroup1 = result->getBlobsParentSons()[root1];

                                // blobs already connected through another path
                                if (root1 == root2) {
                                    VLOG(5) << "blob_" << blob->getId() << "& blob_" << otherBlob->getId()
                                            << " are already connected through another path. Common root is : blob_"
                                            << root1->getId();

                                    //are we done merging the whole blob?
                                    auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                          [](uint32_t i, const Blob *b) {
                                                                              return b->getMergeCount() + i;
                                                                          });

                                    //if yes, record the blobgroup as final
                                    if (mergeLeftCount == 0) {
                                        result->getFinalBlobsParentSons()[root1] = blobGroup1;
                                        result->getBlobsParentSons().erase(root1);
                                    }
                                }
                                // we build a blob group from connected blob groups
                                else {
                                    auto root = uf.unionElements(blob, otherBlob); //pick one root as the new root
                                    auto blobGroup2 = result->getBlobsParentSons()[root2];
                                    blobGroup1.splice(blobGroup1.begin(), blobGroup2); //merge sons


                                    //are we done merging the whole blob?
                                    auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                          [](uint32_t i, const Blob *b) {
                                                                              return b->getMergeCount() + i;
                                                                          });

                                    //if yes, record the blobgroup as final
                                    if (mergeLeftCount == 0) {
                                        result->getFinalBlobsParentSons().insert({root, blobGroup1});
                                        result->getBlobsParentSons().erase(root1);
                                        result->getBlobsParentSons().erase(root2);
                                    //otherwise keep track of the merged blobgroup
                                    } else {
                                        result->getBlobsParentSons()[root] = blobGroup1;
                                        if (root == root2) {
                                            result->getBlobsParentSons().erase(root1);
                                        } else {
                                            result->getBlobsParentSons().erase(root2);
                                        }
                                    }
                                }
                                break;
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
    void addBlobsToNextLevelMerge(std::shared_ptr<ViewAnalyse> currentView,
                                  std::pair<uint32_t, uint32_t> tileCoordinates,
                                  std::pair<uint32_t, uint32_t> contiguousBlockCoordinates) {
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


    void mergeAllHoles(std::shared_ptr<ViewAnalyseBlock> data) {
        auto views = data->getViewAnalyses();

        //first copy all the blob groups we will need to work with
        for(auto &entry : views) {
            auto view = entry.second;
            for(auto parentSon : view->getHolesParentSons()) {
                result->getHolesParentSons().insert(parentSon);
            }
        }

        switch (data->getType()){
            case ViewAnalyseBlock::REGULAR4: {

                //TOP LEFT CORNER
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;
                mergeHoles(view, views[{row + 1, col}]); //BOTTOM
                mergeHoles(view, views[{row, col + 1}]); //RIGHT
                mergeHoles(view, views[{row + 1, col + 1}]); //BOTTOM-RIGHT

                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT

//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
//                addToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

                //TOP RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                mergeHoles(view, views[{row + 1, col}]); //BOTTOM
                mergeHoles(view, views[{row + 1, col - 1}]); //BOTTOM-LEFT

                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-LEFT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                mergeHoles(view, views[{row, col + 1}]); // RIGHT

                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addHolesToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM-RIGHT CORNER
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

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

                mergeHoles(view, views[{row + 1, col}]); //BOTTOM

                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

                //BOTTOM
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT

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

                mergeHoles(view, views[{row, col + 1}]); //RIGHT

                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); // LEFT
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row + 1, col - 1}, {data->getRow() + 1, data->getCol() - 1}); //BOTTOM-LEFT

//                addToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
//                addToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT


                //RIGHT
                it++;
                coordinates = it->first;
                row = coordinates.first, col = coordinates.second;
                view = it->second;
                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT

//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
//                addToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                break;
            }
            case ViewAnalyseBlock::SINGLE1: {
                auto it = views.begin();
                auto coordinates = it->first;
                auto row = coordinates.first, col = coordinates.second;
                auto view = it->second;
                addHolesToNextLevelMerge(view, {row - 1, col}, {data->getRow() - 1, data->getCol()}); //TOP
                addHolesToNextLevelMerge(view, {row, col - 1}, {data->getRow(), data->getCol() - 1}); //LEFT
                addHolesToNextLevelMerge(view, {row, col + 1}, {data->getRow(), data->getCol() + 1}); //RIGHT
                addHolesToNextLevelMerge(view, {row + 1, col}, {data->getRow() + 1, data->getCol()}); //BOTTOM
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //TOP-LEFT
                addHolesToNextLevelMerge(view, {row - 1, col + 1}, {data->getRow() - 1, data->getCol() + 1}); //TOP-RIGHT
                addHolesToNextLevelMerge(view, {row + 1, col + 1}, {data->getRow() + 1, data->getCol() + 1}); //BOTTOM-RIGHT
                addHolesToNextLevelMerge(view, {row - 1, col - 1}, {data->getRow() - 1, data->getCol() - 1}); //BOTTOM-LEFT
            }
        }
    }

    /**
     * Merge blobs at the border of two contiguous tiles.
     */
    void mergeHoles(std::shared_ptr<ViewAnalyse> v1, std::shared_ptr<ViewAnalyse> v2){
        VLOG(3) << result->getLevel() << ": (" << result->getRow() << ", " << result->getCol() <<  "). " << "merging tile at level " << result->getLevel() - 1 << " : " << "(" << v1->getRow() << "," << v1->getCol() << ") & (" << v2->getRow() << "," << v2->getCol() << ")"  << " ...";

        //find corresponding merge regions in each view
        auto blobsToMerge = v1->getHolesToMerge()[{v2->getRow(),v2->getCol()}];
        auto otherBlobsToMerge = v2->getHolesToMerge()[{v1->getRow(),v1->getCol()}];

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

        bool merged;

        //for each blob's border pixel to merge, find the corresponding blobs to merge in the adjacent tile
        for(auto &blobPixelPair : blobsToMerge) {
            auto blob = blobPixelPair.first;
            for(auto coordinates : blobPixelPair.second) {

                merged = false;

                for(auto otherBlobPixelPair : otherBlobsToMerge) {
                    auto otherBlob = otherBlobPixelPair.first;
                    if (otherBlobPixelPair.first->isPixelinFeature(coordinates.first, coordinates.second)) {

                        merged = true;

                        //merge the blob groups
                        VLOG(5) << "Creating merged blob from blobsToMerge : blob_" << blob->getId() << ", blob_" << otherBlob->getId();
                        UnionFind<Blob> uf{};
                        blob->decreaseMergeCount();
                        otherBlob->decreaseMergeCount();
                        auto root1 = uf.find(blob);
                        auto root2 = uf.find(otherBlob);

                        auto blobGroup1 = result->getHolesParentSons()[root1];

                        // blobs already connected through another path
                        if (root1 == root2) {
                            VLOG(5) << "blob_" << blob->getId() << "& blob_" << otherBlob->getId()
                                    << " are already connected through another path. Common root is : blob_"
                                    << root1->getId();

                            //are we done merging the whole blob?
                            auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                  [](uint32_t i, const Blob *b) {
                                                                      return b->getMergeCount() + i;
                                                                  });

                            //if yes, record the blobgroup as final
                            if (mergeLeftCount == 0) {
                                result->getFinalHolesParentSons().insert({root1, blobGroup1});
                                result->getHolesParentSons().erase(root1);
                            }
                        }
                            // we build a blob group from connected blob groups
                        else {
                            auto root = uf.unionElements(blob, otherBlob); //pick one root as the new root
                            auto blobGroup2 = result->getHolesParentSons()[root2];
                            blobGroup1.splice(blobGroup1.begin(), blobGroup2); //merge sons


                            //are we done merging the whole blob?
                            auto mergeLeftCount = std::accumulate(blobGroup1.begin(), blobGroup1.end(), 0,
                                                                  [](uint32_t i, const Blob *b) {
                                                                      return b->getMergeCount() + i;
                                                                  });

                            //if yes, record the blobgroup as final
                            if (mergeLeftCount == 0) {
                                result->getFinalHolesParentSons().insert({root, blobGroup1});
                                result->getHolesParentSons().erase(root1);
                                result->getHolesParentSons().erase(root2);
                                //otherwise keep track of the merged blobgroup
                            } else {
                                result->getHolesParentSons()[root] = blobGroup1;
                                if (root == root2) {
                                    result->getHolesParentSons().erase(root1);
                                } else {
                                    result->getHolesParentSons().erase(root2);
                                }
                            }
                        }
                        break;
                    }
                }

                if(!merged) {
               //     blob->decreaseMergeCount();
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
    void addHolesToNextLevelMerge(std::shared_ptr<ViewAnalyse> currentView,
                                  std::pair<uint32_t, uint32_t> tileCoordinates,
                                  std::pair<uint32_t, uint32_t> contiguousBlockCoordinates) {

        auto level = currentView->getLevel();
        if((tileCoordinates.first >= pyramid.getNumTileRow(level) || tileCoordinates.second >= pyramid.getNumTileCol(level)) ||
           contiguousBlockCoordinates.first >= pyramid.getNumTileRow(level + 1) || contiguousBlockCoordinates.second >= pyramid.getNumTileCol(level + 1))
        {
            return;
        }

        if(!(currentView->getHolesToMerge()[tileCoordinates]).empty()){
            VLOG(5) << "level: " << result->getLevel() << " - block : (" << result->getRow() << ", " << result->getCol() <<  "). "  << " - add merge with block : " << "(" << contiguousBlockCoordinates.first << "," << contiguousBlockCoordinates.second << ")" << "- original tile : " << "(" << tileCoordinates.first << "," << tileCoordinates.second << ")" << "need to merge " << currentView->getHolesToMerge().size() << " at the next level";
            for(auto blobToPixelEntry : currentView->getHolesToMerge()[tileCoordinates]) {
                result->getHolesToMerge()[contiguousBlockCoordinates].insert(blobToPixelEntry);
            }
        }
    }



    //Managed by HTGS, no need to delete
    ViewAnalyse* result{};

    pb::Pyramid pyramid;

    SegmentationOptions *options;

    AABBTree<Blob>* tree;

};

}

#endif //NEWEGT_MERGEBLOB_H
