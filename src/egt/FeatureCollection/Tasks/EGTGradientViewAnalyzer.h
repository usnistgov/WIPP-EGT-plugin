//
// Created by gerardin on 7/19/19.
//

#ifndef EGT_EGTGRADIENTVIEWANALYZER_H
#define EGT_EGTGRADIENTVIEWANALYZER_H

// NIST-developed software is provided by NIST as a public service.
// You may use, copy and distribute copies of the  software in any  medium,
// provided that you keep intact this entire notice. You may improve,
// modify and create derivative works of the software or any portion of the
// software, and you may copy and distribute such modifications or works.
// Modified works should carry a notice stating that you changed the software
// and should note the date and nature of any such change. Please explicitly
// acknowledge the National Institute of Standards and Technology as the
// source of the software.
// NIST-developed software is expressly provided "AS IS." NIST MAKES NO WARRANTY
// OF ANY KIND, EXPRESS, IMPLIED, IN FACT  OR ARISING BY OPERATION OF LAW,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT AND DATA ACCURACY. NIST
// NEITHER REPRESENTS NOR WARRANTS THAT THE OPERATION  OF THE SOFTWARE WILL
// BE UNINTERRUPTED OR ERROR-FREE, OR THAT ANY DEFECTS WILL BE CORRECTED. NIST
// DOES NOT WARRANT  OR MAKE ANY REPRESENTATIONS REGARDING THE USE OF THE
// SOFTWARE OR THE RESULTS THEREOF, INCLUDING BUT NOT LIMITED TO THE
// CORRECTNESS, ACCURACY, RELIABILITY, OR USEFULNESS OF THE SOFTWARE.
// You are solely responsible for determining the appropriateness of using
// and distributing the software and you assume  all risks associated with
// its use, including but not limited to the risks and costs of program
// errors, compliance  with applicable laws, damage to or loss of data,
// programs or equipment, and the unavailability or interruption of operation.
// This software is not intended to be used in any situation where a failure
// could cause risk of injury or damage to property. The software developed
// by NIST employees is not subject to copyright protection within
// the United States.

/// @file EGTViewAnalyzer.h
/// @author Alexandre Bardakoff - Timothy Blattner - Antoine Gerardin
/// @date   7/19/19
/// @brief Task which analyse a view to find the different blob into it. It can also create a mask.

#include <cstdint>
#include <unordered_set>
#include<egt/FeatureCollection/Data/Blob.h>
#include <egt/utils/Utils.h>
#include <egt/api/SegmentationOptions.h>
#include <egt/api/DerivedSegmentationParams.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include <egt/FeatureCollection/Data/ViewOrViewAnalyse.h>
#include <egt/api/DataTypes.h>
#include <egt/data/GradientView.h>
#include <egt/utils/FeatureExtraction.h>

namespace egt {


/**
  * @class EGTGradientViewAnalyser EGTGradientViewAnalyser.h <egt/FeatureCollection/Tasks/EGTGradientViewAnalyser.h>
  *
  * @brief View Analyser, HTGS task, take a GradientView and produce a
  * ViewAnalyse for the BlobMerger.
  *
  * @details HTGS task which segments an image tile using a flood algorithm
  * (https://en.wikipedia.org/wiki/Flood_fill) in a view to find the different
  * connected pixels called blobs. Two connected rules are proposed:
  * _4 (North, South, East, West)
  * _8 (North, North-East, North-West, South, South-East, South-West, East, West)
  * Blobs can represent Object (Foreground color) or Holes (Background color).
  *
  * @tparam UserType File pixel type
  **/
    template<class UserType>
    class EGTGradientViewAnalyzer : public htgs::ITask<GradientView<UserType>,
            ViewOrViewAnalyse<UserType>> {

    public:

        enum Color { FOREGROUND, BACKGROUND };

        EGTGradientViewAnalyzer(size_t numThreads,
                const uint32_t imageHeight,
                const uint32_t imageWidth,
                const int32_t tileHeight,
                const int32_t tileWidth,
                const uint8_t rank,
                const UserType background,
                SegmentationOptions* options,
                DerivedSegmentationParams<UserType>& params
                )
                : ITask<GradientView<UserType>, ViewOrViewAnalyse<UserType>>(numThreads),
                  _imageHeight(imageHeight),
                  _imageWidth(imageWidth),
                  _tileHeight(tileHeight),
                  _tileWidth(tileWidth),
                  _rank(rank),
                  _background(background),
                  _segmentationOptions(options),
                  _segmentationParams(params),
                  _vAnalyse(nullptr) {
            _visited = std::vector<bool>((unsigned long)(_tileWidth * _tileHeight), false);
        }


        /// \brief Execute the task, do a view analyse
        /// \param view View given by the FI
        void executeTask(std::shared_ptr<GradientView<UserType>> view)
        override {
            _view = view->getGradientView()->get();
            _originalView = view->getOriginalView();
            //FOR EACH NEW TILE WE NEED TO RESET THE TASK STATE SINCE MANY TILES CAN BE PROCESSED BY ONE TASK
            _vAnalyse = new ViewAnalyse(_view->getRow(), _view->getCol(), _view->getPyramidLevel()); //OUTPUT
            _toVisit.clear(); //clear queue that keeps track of neighbors to visit when flooding
            _visited.assign(_visited.size(), false); //clear container that keeps track of all visited pixels in a pass through the image.
            _currentBlob = nullptr;
            _tileHeight = _view->getTileHeight();
            _tileWidth = _view->getTileWidth();
            _imageSize = _tileWidth * _tileHeight;

            visitedCount = 0;
            run(BACKGROUND); //find holes
            auto backgroundPixelCount = visitedCount;
            visitedCount = 0;
            run(FOREGROUND); //find objects
            auto foregroundPixelCount = visitedCount;
            assert(_imageSize == backgroundPixelCount + foregroundPixelCount);

            //if MASK_ONLY, we return the view with all pixel set to 0 (background) or 255 (foreground).
            //Let's not forget to delete the viewAnalyse since we are done with it.
            if(_segmentationOptions->MASK_ONLY) {
                VLOG(5) << "segmenting tile (" << _view->getRow() << " , " << _view->getCol() << ") :";
                VLOG(5) << "holes turned to foreground : " << holeRemovedCount;
                VLOG(5) << "objects removed because too small: " << objectRemovedCount;
                delete _vAnalyse;
                delete view->getOriginalView();
                this->addResult(new ViewOrViewAnalyse<UserType>(view->getGradientView()));
            }
            //we return a ViewAnalyse to be merged. Let's not forget to release the view since we are done with it.
            else {
                VLOG(5) << "segmenting tile (" << _view->getRow() << " , " << _view->getCol() << ") :";
                VLOG(5) << "holes turned to foreground : " << holeRemovedCount;
                VLOG(5) << "objects removed because too small: " << objectRemovedCount;
                VLOG(5) << "holes we keep track of in the merge: " << _vAnalyse->getHoles().size();
                VLOG(5) << "holes to merge: " << _vAnalyse->getHolesToMerge().size();
                VLOG(5) << "objects found: " << _vAnalyse->getBlobs().size();
                VLOG(5) << "objects to merge: " << _vAnalyse->getToMerge().size();
                view->getGradientView()->releaseMemory();
                this->addResult(new ViewOrViewAnalyse<UserType>(_vAnalyse));
            }
        }

        /// \brief View analyser copy function
        /// \return A new View analyser
        EGTGradientViewAnalyzer<UserType> *copy() override {
            auto viewAnalyzer = new EGTGradientViewAnalyzer<UserType>(this->getNumThreads(),
                                                           _imageHeight,
                                                           _imageWidth,
                                                           _tileHeight,
                                                           _tileWidth,
                                                           _rank,
                                                           _background,
                                                           _segmentationOptions,
                                                           _segmentationParams);
            return viewAnalyzer;
        }

        /// \brief Get task name
        /// \return Task name
        std::string getName() override {
            return "EGT View analyzer";
        }


    private:

        void run(Color blobColor){
//            printArray<UserType>("", _view->getData(), _view->getViewWidth(), _view->getViewHeight(), 4);

            for (int32_t row = 0; row < _tileHeight; ++row) {
                for (int32_t col = 0; col < _tileWidth; ++col) {

                    //WE ENTER HERE ONCE WE HAVE CREATED A BLOB THAT HAS NEIGHBORS OF THE SAME COLOR
                    while (!_toVisit.empty()) {
                        expandBlob(blobColor);
                    }

                    //WE ENTER HERE WHEN WE ARE DONE EXPANDING THE CURRENT BLOB
                    if (_currentBlob != nullptr) {
                        blobCompleted(blobColor);
                    }

                    //UNLESS WE ARE DONE EXPLORING WE CREATE A ONE PIXEL BLOB AND EXPLORE ITS NEIGHBORS
                    if (!visited(row, col) && getColor(row, col) == blobColor) {
                        createBlob(row,col, blobColor);
                    }
                }
            }

            //If the last pixel was a blob by itself, it has not been saved yet.
            if (_currentBlob != nullptr) {
                blobCompleted(blobColor);
            }
       }

       /**
        * We have a queue of pixels to visit. Dequeue the first one and explore its neighbors.
        * We use connectivity 4 for background and connectivity 8 for foreground.
        * @param blobColor
        */
        void expandBlob(Color blobColor){
            auto neighbourCoord = *_toVisit.begin();
            _toVisit.erase(_toVisit.begin());
            //mark pixel as visited so we don't look at it again
            markAsVisited(neighbourCoord.first,
                          neighbourCoord.second);

            if(_segmentationOptions->MASK_ONLY){
                auto maskValue = (getColor(neighbourCoord.first, neighbourCoord.second) == FOREGROUND) ? 255 : 0;
                _view->setPixel(neighbourCoord.first,neighbourCoord.second, maskValue);
            }

            _currentBlob->addPixel(
                    _view->getGlobalYOffset() + neighbourCoord.first,
                    _view->getGlobalXOffset() + neighbourCoord.second);

            if (blobColor == BACKGROUND) {
                analyseNeighbour4(neighbourCoord.first, neighbourCoord.second, blobColor);
            }
            else {
                analyseNeighbour4(neighbourCoord.first, neighbourCoord.second, blobColor);
            }
        }

        /**
         * Decide what to do once we have completed a blob.
         * @param blobColor
         */
        void blobCompleted(Color blobColor) {

                if(_currentBlob->isToMerge()){
                    flattenPixelToMerge(_currentBlob, blobColor);
                }

                //background and foreground blobs are not handled the same way
                if(blobColor == BACKGROUND) {

                    //we know this hole is on the border, so we keep track of it for the merge.
                    if (_currentBlob->isToMerge()) {
                        if(!_segmentationOptions->MASK_ONLY) {
                            _currentBlob->compactBlobDataIntoFeature();
                            _vAnalyse->insertHole(_currentBlob);
                        }
                    }
                    //for local hole, decide if we fill it up.
                    else {
                        bool keepHole = false;
                        auto area = _currentBlob->getCount();

                        if( _segmentationOptions->disableIntensityFilter) {
                            keepHole = computeKeepHoleAreaOnlyCriteria<UserType>(area, _segmentationOptions, _segmentationParams);
                        }
                        else {
                            UserType meanIntensity = computeMeanIntensity();
                            keepHole = computeKeepHoleCriteria<UserType>(area, meanIntensity, _segmentationOptions, _segmentationParams);
                        }

                        if(!keepHole) {
                            //TODO WE COULD ALSO FILL HOLE IF THE BOUNDING BOX IS NOT TOUCHING THE BORDER (THE BLOB IS FULLY CONTAINED INSIDE)
                            //fillUpHole();
                            _currentBlob->compactBlobDataIntoFeature();
                            _vAnalyse->insertBlob(_currentBlob);
                            holeRemovedCount++;
                            //TODO now we need to merge it with (one of) the surrounding blob(s)

                        }
                        else {
                            delete _currentBlob;

                        }
                    }
                }
                else {
                    //we delete small objects
                    if (!_currentBlob->isToMerge() && _currentBlob->getCount() < _segmentationOptions->MIN_OBJECT_SIZE) {

                        if(_segmentationOptions->MASK_ONLY){
                            for (auto it = _currentBlob->getRowCols().begin();
                                 it != _currentBlob->getRowCols().end(); ++it) {
                                auto pRow = it->first;
                                for (auto pCol : it->second) {
                                    auto xOffset = _view->getGlobalXOffset();
                                    auto yOffset = _view->getGlobalYOffset();
                                    _view->setPixel(pRow - yOffset, pCol - xOffset,0);
                                }
                            }
                        }
                        delete _currentBlob;
                        objectRemovedCount++;
                    }
                    // we keep track of the others
                    else{
                        if(!_segmentationOptions->MASK_ONLY) {
                            _currentBlob->compactBlobDataIntoFeature();
                            _vAnalyse->insertBlob(_currentBlob);
                        }
                    }
                }

            _currentBlob = nullptr;
        }


//        static bool sortByCol(const std::pair<uint32_t,uint32_t > &a,
//                       const std::pair<uint32_t,uint32_t > &b)
//        {
//            return (a.second < b.second);
//        }

        void flattenPixelToMerge(Blob* blob, Color blobColor) {

            blob->setMergeCount(0); //we reset the counter

            std::map<Coordinate, std::unordered_map<Blob *, std::list<Coordinate>>>::iterator mergeIterator, endIterator;
            if(blobColor == BACKGROUND) {
                mergeIterator = _vAnalyse->getHolesToMerge().begin();
                endIterator = _vAnalyse->getHolesToMerge().end();
            }
            else {
                mergeIterator = _vAnalyse->getToMerge().begin();
                endIterator = _vAnalyse->getToMerge().end();
            }

            while(mergeIterator != endIterator) {

                std::unordered_map<Blob *, std::list<Coordinate>>::iterator blobMapIt = mergeIterator->second.find(blob); //iterator on the entry (blob => list of pixel coordinates)

                if(blobMapIt != mergeIterator->second.end()){
                    std::list<Coordinate> & listToFlat =  blobMapIt->second;
                    auto originalSize = listToFlat.size();
                    listToFlat.sort();

                    auto prev = listToFlat.front();
                    auto it = listToFlat.begin()++;

                    //we are flattening a row of pixels
                    if(listToFlat.front().first == listToFlat.back().first){

                        while (it != listToFlat.end()) {
                            if (prev.second + 1 == (*it).second) {
                                prev = (*it);
                                it = listToFlat.erase(it);
                            } else {
                                prev = (*it);
                                it++;
                            }
                        }
                    }
                    //we are flattening a column of pixels
                    else {
                        while (it != listToFlat.end()) {
                            if (prev.first + 1 == (*it).first) {
                                prev = (*it);
                                it = listToFlat.erase(it);
                            } else {
                                prev = (*it);
                                it++;
                            }
                        }
                    }

                    //add the number of merge to perform in one direction to the total number of merges to perform
                    blob->setMergeCount(blob->getMergeCount() + (uint32_t)listToFlat.size());

//                    VLOG(3) << "blob : " << blob->getTag() << " - Flattened border pixels from " << originalSize << " down to " << blob->getMergeCount();
                }

                mergeIterator++;
            }

        }


/**
         * Create a new blob at a given position and try to expand it.
         */
        void createBlob(int32_t row, int32_t col, Color blobColor) {
            markAsVisited(row, col);
            //add pixel to a new blob (we are recording the global position)
            _currentBlob = new Blob(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col, _view->getRow(), _view->getCol());
            _currentBlob->addPixel(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col);

            //look at its neighbors
            if (blobColor == BACKGROUND) {
                analyseNeighbour4(row, col, blobColor);
            }
            else {
                analyseNeighbour8(row, col, blobColor);
            }
        }

        /// \brief Analyse the neighbour of a pixel for a 4-connectivity
        void analyseNeighbour4(int32_t row, int32_t col, Color color) {

            auto globalRow = row + _view->getGlobalYOffset();
            auto globalCol = col + _view->getGlobalXOffset();

            //Explore neighbors in all directions to see if they need to be added to the blob.
            //Top Pixel
            if (row >= 1) {
                if (!visited(row - 1, col) && getColor(row - 1, col) == color) {
                    _toVisit.emplace(row - 1, col);
                }
            }
            // Bottom pixel
            if (row + 1 < _tileHeight) {
                if (!visited(row + 1, col) && getColor(row + 1, col) == color) {
                    _toVisit.emplace(row + 1, col);
                }
            }
            // Left pixel
            if (col >= 1) {
                if (!visited(row, col - 1) && getColor(row, col - 1) == color) {
                    _toVisit.emplace(row, col - 1);
                }
            }
            // Right pixel
            if ( (col + 1) < _tileWidth) {
                if (!visited(row, col + 1) && getColor(row, col + 1) == color) {
                    _toVisit.emplace(row, col + 1);
                }
            }

            //WE DON'T NEED MERGING IF WE GENERATE ONLY THE MASK
            if(_segmentationOptions->MASK_ONLY){
                return;
            }

            //Check if the blob in this tile belongs to a bigger blob extending several tiles.
            //We only to look at EAST and SOUTH so we can later merge only once, TOP to BOTTOM and LEFT to RIGHT.

            // Add blob to merge list if tile bottom pixel has the same value than the view pixel below (continuity)
            // test that we are also not at the edge of the full image.
            if (row + 1 == _tileHeight && globalRow + 1 != _imageHeight) {
                if (getColor(row + 1, col) == color) {
                    auto coords = std::pair<int32_t, int32_t>(globalRow + 1, globalCol);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge({_view->getRow() + 1, _view->getCol()}, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge({_view->getRow() + 1, _view->getCol()}, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }
            // Add blob to merge list if tile right pixel has the same value than the view pixel on its right (continuity)
            // test that we are also not at the edge of the full image.
            if (col + 1 == _tileWidth && globalCol + 1 != _imageWidth) {
                if (getColor(row, col + 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow,globalCol + 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge({_view->getRow(), _view->getCol() + 1}, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge({_view->getRow(), _view->getCol() + 1}, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            //look at the pixel on the left. We need to set a flag so we know how to filter this blob.
            if (col == 0 && col + _view->getGlobalXOffset() != 0) {
                if (getColor(row, col - 1) == color) {
                    auto coords =std::pair<int32_t, int32_t>(globalRow,globalCol - 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge({_view->getRow(), _view->getCol() - 1}, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge({_view->getRow(), _view->getCol() - 1}, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();

                }
            }

            //look at the pixel above. We need to set a flag so we know how to filter this blob.
            if (row == 0 && row + _view->getGlobalYOffset() != 0) {
                if (getColor(row - 1, col) == color) {
                    auto coords =std::pair<int32_t, int32_t>(globalRow - 1,globalCol);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge({_view->getRow() - 1, _view->getCol()}, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge({_view->getRow() - 1, _view->getCol()}, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }
        }

        void analyseNeighbour8(int32_t row, int32_t col, Color color) {

            // Analyse the pixels around the pixel
            int32_t
                    minRow = std::max(0, row - 1),
                    maxRow = std::min(_view->getTileHeight(), row + 2),
                    minCol = std::max(0, col - 1),
                    maxCol = std::min(_view->getTileWidth(), col + 2);

            for (int32_t rowP = minRow; rowP < maxRow; ++rowP) {
                for (int32_t colP = minCol; colP < maxCol; ++colP) {
                    if (!visited(rowP, colP)  && getColor(rowP, colP) == color) {
                        _toVisit.emplace(rowP, colP);
                    }
                }
            }

            //WE DON'T NEED MERGING IF WE GENERATE ONLY THE MASK
            if(_segmentationOptions->MASK_ONLY){
                return;
            }

            auto globalRow = row + _view->getGlobalYOffset();
            auto globalCol = col + _view->getGlobalXOffset();

            //Check if the blob in this tile belongs to a bigger blob extending several tiles.
            //We only to look at EAST and SOUTH so we can later merge only once, TOP to BOTTOM and LEFT to RIGHT.

            //test whether pixel is at the image border AND is not at the edge of the full image.
            bool onTileBottomBorder = (row + 1 == _tileHeight && globalRow + 1 != _imageHeight);
            bool onTileRightBorder = (col + 1 == _tileWidth && globalCol + 1 != _imageWidth);
            bool onTileTopBorder = (row == 0 && globalRow != 0);
            bool onTileLeftBorder = (col == 0 && globalCol != 0);

            if (onTileBottomBorder) {
                if (getColor(row + 1, col) == color) {
                    auto coords = std::pair<int32_t, int32_t>(globalRow + 1, globalCol);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() + 1, _view->getCol());

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            if (onTileRightBorder) {
                if (getColor(row, col + 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow,globalCol + 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow(), _view->getCol() + 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords,_currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords,_currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            //Bottom right pixel
            if (onTileRightBorder && onTileBottomBorder) {
                if (getColor(row + 1, col + 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow + 1,globalCol + 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() + 1, _view->getCol() + 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            //Top right pixel (we could have alternatively check the bottom left)
            if (onTileRightBorder && onTileTopBorder) {
                if (getColor(row - 1, col + 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow - 1,globalCol + 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() - 1, _view->getCol() + 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }


            // We need to set a flag so we know how to filter this blob.

            if (onTileTopBorder) {
                if (getColor(row - 1, col) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow - 1,globalCol);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() - 1, _view->getCol());

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            if (onTileLeftBorder) {
                if (getColor(row, col - 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow,globalCol - 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow(), _view->getCol() - 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            if (onTileLeftBorder && onTileTopBorder) {
                if (getColor(row - 1, col - 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow - 1,globalCol - 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() - 1, _view->getCol() - 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

            if (onTileLeftBorder && onTileBottomBorder) {
                if (getColor(row + 1, col - 1) == color) {

                    auto coords =std::pair<int32_t, int32_t>(globalRow + 1,globalCol - 1);
                    auto tileCoords = std::pair<int32_t, int32_t>(_view->getRow() + 1, _view->getCol() - 1);

                    if(color == BACKGROUND){
                        _vAnalyse->addHolesToMerge(tileCoords, _currentBlob, coords);
                    }
                    else {
                        _vAnalyse->addToMerge(tileCoords, _currentBlob, coords);
                    }
                    _currentBlob->increaseMergeCount();
                }
            }

        }


        /**
         * @return the mean intensity for this blob.
         */
        UserType computeMeanIntensity() {
            auto xOffset = _view->getGlobalXOffset();
            auto yOffset = _view->getGlobalYOffset();
            uint64_t sum = 0, count = 0;

            for(const auto &entry : _currentBlob->getRowCols()) {
                auto row = entry.first;
                for(const auto col : entry.second) {
                    sum += _originalView[(row - yOffset) * _tileWidth + (col - xOffset)];
                    count ++;
                }
            }
            auto intensity = (UserType)(sum / count);

            VLOG(5) << "hole (" << _currentBlob->getTag()  << ") mean intensity "  << intensity;

            return intensity;
        }

        /**
         * Set every pixel value to foreground and make sure those pixels are visited again in the object detection step.
         */
        void fillUpHole() {
            for (auto it = _currentBlob->getRowCols().begin();
                 it != _currentBlob->getRowCols().end(); ++it) {
                auto pRow = it->first;
                for (auto pCol : it->second) {
                    auto xOffset = _view->getGlobalXOffset();
                    auto yOffset = _view->getGlobalYOffset();

                    markAsUnvisited(pRow - yOffset, pCol - xOffset);

                    if (_segmentationOptions->MASK_ONLY) {
                        _view->setPixel(pRow - yOffset, pCol - xOffset, 255);
                    } else {
                        _view->setPixel(pRow - yOffset, pCol - xOffset, _background + 1);
                    }
                }
            }

            VLOG(5) << "hole (" << _currentBlob->getTag()  << ") filled up : "  << _currentBlob->getCount() << " pixels turned into foreground.";
        }


        void markAsVisited(int32_t row, int32_t col) {
            _visited[row * _tileWidth + col] = true;
            visitedCount++;
        }

        void markAsUnvisited(int32_t row, int32_t col) {
            _visited[row * _tileWidth + col] = false;
            visitedCount--;
        }

        inline bool visited(int32_t row, int32_t col){
            return _visited[row * _tileWidth + col];
        }

        Color getColor(int32_t row, int32_t col) {
            if(_view->getPixel(row,col) > _background){
                return Color::FOREGROUND;
            }
            return Color::BACKGROUND;
        }



        fi::View<UserType>* _view;
        UserType* _originalView;

        const uint8_t _rank{}; ///< Rank to the connectivity: < 4=> 4-connectivity, 8=> 8-connectivity


        std::vector<bool> _visited{}; ///keep track of every pixel we looked at.
        std::set<Coordinate>_toVisit{}; /// for a current position, keep track of the neighbors that needs visit.


        Blob
                *_currentBlob = nullptr;      ///< Current blob

        int32_t
                _tileHeight{},                ///< Tile actual height
                _tileWidth{};                 ///< Tile actual width
        int64_t _imageSize{};


        UserType _background{}; /// Threshold value chosen to represent the value above which we have a foreground pixel.

        ///We need those if we are trying to merge blobs globally.
        const uint32_t
                _imageHeight{},               ///< Mask height
                _imageWidth{};                ///< Mask width

        SegmentationOptions* _segmentationOptions{};
        DerivedSegmentationParams<UserType> _segmentationParams{};


        ViewAnalyse *_vAnalyse = nullptr;         ///< Current view analyse

        uint32_t objectRemovedCount = 0,
                 holeRemovedCount = 0,
                 visitedCount = 0;


    };


}

#endif //EGT_EGTGRADIENTVIEWANALYZER_H
