//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-14.
//

#ifndef EGT_VIEWANALYZER_H
#define EGT_VIEWANALYZER_H

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

/// @file ViewAnalyser.h
/// @author Alexandre Bardakoff - Timothy Blattner
/// @date  4/6/17
/// @brief Task wich analyse a view to find the different blob into it.

#include <cstdint>
#include <unordered_set>
#include<egt/FeatureCollection/Data/Blob.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>
#include <egt/utils/Utils.h>

using namespace fc;

namespace egt {
/// \namespace fc FeatureCollection namespace

/**
  * @class ViewAnalyser ViewAnalyser.h <FastImage/FeatureCollection/Tasks/ViewAnalyser.h>
  *
  * @brief View Analyser, HTGS task, take a FastImage view and produce a
  * ViewAnalyse to the BlobMerger.
  *
  * @details HTGS tasks, which run a flood algorithm
  * (https://en.wikipedia.org/wiki/Flood_fill) in a view to find the different
  * connected pixels called blob. Two connected rules are proposed:
  * _4 (North, South, East, West)
  * _8 (North, North-East, North-West, South, South-East, South-West, East, West)
  *
  * @tparam UserType File pixel type
  **/
    template<class UserType>
    class ViewAnalyser : public htgs::ITask<htgs::MemoryData<fi::View<UserType>>,
            ViewAnalyse> {
    public:
        /// \brief View analyser constructor, create a view analyser from the mask
        /// information
        /// \param numThreads Number of threads the view analyser will be executed in
        /// parallel
        /// \param fi Mask FI
        /// \param rank Rank to the connectivity: 4=> 4-connectivity, 8=>
        /// 8-connectivity
        /// \param background Background value
        ViewAnalyser(size_t numThreads,
                     fi::FastImage<UserType> *fi,
                     const uint8_t &rank,
                     const UserType &background)
                : ITask<htgs::MemoryData<fi::View<UserType>>, ViewAnalyse>(numThreads),
                  _background(background),
                  _fi(fi),
                  _imageHeight(fi->getImageHeight()),
                  _imageWidth(fi->getImageWidth()),
                  _rank(rank),
                  _vAnalyse(nullptr) {
            _visited = std::vector<bool>((fi->getTileWidth(0) * fi->getTileHeight(0)));
            _visited.flip();

        }


        inline bool visited(int32_t row, int32_t col){
            return _visited[row * _view->getTileWidth() + col];
        }

        inline bool needVisit(int32_t row, int32_t col, bool color) {
            return !visited(row,col) && sameColor(row,col, color);
        }

        void markAsVisited(int32_t row, int32_t col) {
            _visited[row * _view->getTileWidth() + col] = true;
        }

        void unmarkAsVisited(int32_t row, int32_t col) {
            _visited[row * _view->getTileWidth() + col] = false;
        }

        bool getColor(uint32_t row, uint32_t col) {
            return _view->getPixel(row,col) > _background;
        }

        bool IS_BACKGROUND = false;

        bool sameColor(uint32_t row, uint32_t col, bool color) {
            return getColor(row, col) == color;
        }

        /// \brief Analyse the neighbour of a pixel for a 4-connectivity
        /// \param row Pixel's row
        /// \param col Pixel's col
        void analyseNeighbour4(int32_t row, int32_t col, bool color) {

            //Explore neighbors in all directions to see if they need to be added to the blob.

            // Top Pixel
            if (row >= 1) {
                if (needVisit(row - 1, col, color)) {
                    _toVisit.emplace(row - 1, col);
                }
            }
            // Bottom pixel
            if (row + 1 < _tileHeight) {
                if (needVisit((row + 1), col, color)) {
                    _toVisit.emplace(row + 1, col);
                }
            }
            // Left pixel
            if (col >= 1) {
                if (needVisit(row, col - 1, color)) {
                    _toVisit.emplace(row, col - 1);
                }
            }
            // Right pixel
            if (col + 1 < _tileWidth) {
                if (needVisit(row, col + 1, color)) {
                    _toVisit.emplace(row, col + 1);
                }
            }

            //Check if the blob in this tile belongs to a bigger blob extending several tiles.
            //We only to look at EAST and SOUTH so we can later merge only once, TOP to BOTTOM and LEFT to RIGHT.

            // Add blob to merge list if tile bottom pixel has the same value than the view pixel below (continuity)
            // test that we are also not at the edge of the full image.
            if (row + 1 == _tileHeight && row + _view->getGlobalYOffset() + 1 != _imageHeight) {
                if (sameColor(row + 1, col, color)) {
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + 1 + _view->getGlobalYOffset(),
                                                  col + _view->getGlobalXOffset()));
                    _currentBlob->setToMerge(true);
                }

            }
            // Add blob to merge list if tile right pixel has the same value than the view pixel on its right (continuity)
            // test that we are also not at the edge of the full image.
            if (col + 1 == _tileWidth && col + _view->getGlobalXOffset() + 1 != _imageWidth) {
                if (sameColor(row + 1, col, color)) {
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + _view->getGlobalYOffset(),
                                                  col + 1 + _view->getGlobalXOffset()));
                }
                _currentBlob->setToMerge(true);
            }

            //TODO do the same with 8 connectivity

            //look at the pixel on the left
            if (col == 0 && col + _view->getGlobalXOffset() != 0) {
                if (sameColor(row, col - 1, color)) {
                    _currentBlob->setToMerge(true);
                }
            }

            //look at the pixel above
            if (row == 0 && row + _view->getGlobalYOffset() != 0) {
                if (sameColor(row - 1 , col, color)) {
                    _currentBlob->setToMerge(true);
                }
            }
        }

        /// \brief Analyse the neighbour of a pixel for a 8-connectivity
        /// \param row Pixel's row
        /// \param col Pixel's col
        void analyseNeighbour8(int32_t row, int32_t col, bool color) {
            int32_t
                    minRow = std::max(0, row - 1),
                    maxRow = std::min(_view->getTileHeight(), row + 2),
                    minCol = std::max(0, col - 1),
                    maxCol = std::min(_view->getTileWidth(), col + 2);

            // Analyse the pixels around the pixel
            for (int32_t rowP = minRow; rowP < maxRow; ++rowP) {
                for (int32_t colP = minCol; colP < maxCol; ++colP) {
                    if (needVisit(rowP, colP, color)) {
                        _toVisit.emplace(rowP, colP);
                    }
                }
            }

            // Add a pixel to to merge if the bottom pixel is outside of the tile,
            // but had a foreground value
            if (row + 1 == _tileHeight
                && row + _view->getGlobalYOffset() + 1 != _imageHeight) {
                if (sameColor(row + 1, col, color)) {
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + 1 + _view->getGlobalYOffset(),
                                                  col + _view->getGlobalXOffset())
                    );
                }
            }

            // Add a pixel to to merge if the right pixel is outside of the tile, but
            // had a foreground value
            if (col + 1 == _tileWidth
                && col + _view->getGlobalXOffset() + 1 != _imageWidth) {
                if((sameColor(row, col + 1, color))){
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + _view->getGlobalYOffset(),
                                                  col + 1 + _view->getGlobalXOffset()));
                }
            }

            // Add a pixel to to merge if the bottom-right pixel is outside of the tile,
            // but had a foreground value
            if ((col == _tileWidth - 1 || row == _tileHeight - 1) &&
                row + _view->getGlobalYOffset() + 1 != _imageHeight
                && col + _view->getGlobalXOffset() + 1 != _imageWidth) {
                if((sameColor(row + 1, col + 1, color))){
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + 1 + _view->getGlobalYOffset(),
                                                  col + 1 + _view->getGlobalXOffset()));
                }
            }

            // Add a pixel to to merge if the top-right pixel is outside of the tile,
            // but had a foreground value
            if ((row == 0 || col == _tileWidth - 1)
                && row + _view->getGlobalYOffset() > 0
                && col + _view->getGlobalXOffset() + 1 != _imageWidth) {
                if((sameColor(row - 1, col + 1, color))){
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row - 1 + _view->getGlobalYOffset(),
                                                  col + 1 + _view->getGlobalXOffset()));
                }
            }
        }



        /// \brief Execute the task, do a view analyse
        /// \param view View given by the FI
        void executeTask(std::shared_ptr<MemoryData<fi::View<UserType>>> view)
        override {
            _view = view->get();
            _tileHeight = _view->getTileHeight();
            _tileWidth = _view->getTileWidth();
            _vAnalyse = new ViewAnalyse();
            _toVisit.clear(); //clear queue that keeps track of neighbors to visit when flooding
            _visited.flip(); //clear container that keeps track of all nodes to be visited in a pass through the image.
            _currentBlob = nullptr;
            _previousBlob = nullptr;

            printArray<uint16_t >("tile_" + std::to_string(_view->getGlobalXOffset()) + "_" + std::to_string(_view->getGlobalYOffset()) ,(uint16_t *)_view->getData(),_view->getViewWidth(),_view->getViewHeight());


            //WE FIRST VISIT BACKGROUND PIXELS TO REMOVE HOLES
            for (int32_t row = 0; row < _tileHeight; ++row) {
                for (int32_t col = 0; col < _tileWidth; ++col) {

                    while (!_toVisit.empty()) {
                        auto neighbourCoord = *_toVisit.begin();
                        _toVisit.erase(_toVisit.begin());
                        //mark pixel as visited so we don't look at it again
                        markAsVisited(neighbourCoord.first,
                                      neighbourCoord.second);
                        bool color = getColor(neighbourCoord.first, neighbourCoord.second);

                        _currentBlob->addPixel(
                                _view->getGlobalYOffset() + neighbourCoord.first,
                                _view->getGlobalXOffset() + neighbourCoord.second);


                        if (_rank == 4) {
                            analyseNeighbour4(neighbourCoord.first, neighbourCoord.second, color);
                        } else {
                            analyseNeighbour8(neighbourCoord.first, neighbourCoord.second, color);
                        }
                    }

                    //we have a completed blob, we need to save it.
                    if (_currentBlob != nullptr) {
                        //we cannot make a decision about removing this hole yet,


                        //we need to merge it with its neighbors to calculate its size.
                        if(_currentBlob->isBackground() && _currentBlob->isToMerge()) {
                            //TODO ignore for now
                            //_vAnalyse->insertBlob(_currentBlob);
                            _previousBlob = _currentBlob;
                            _currentBlob = nullptr;
                        }
                        else if (_currentBlob->getCount() < MIN_HOLE_SIZE){
                            VLOG(1) << "hole detected! Remove.";
                            for(auto it = _currentBlob->getRowCols().begin(); it != _currentBlob->getRowCols().end(); ++it ){
                                auto row = it->first;
                                for(auto col : it->second) {
                                    auto xOffset = _view->getGlobalXOffset();
                                    auto yOffset = _view->getGlobalYOffset();
                                    unmarkAsVisited(row - yOffset,col - xOffset);
                                    _view->setPixel(row - yOffset,col - xOffset, _background + 1);
                                }
                            }
                        }
                    }

                    //Find the next blob to create
                    if (!visited(row, col) & getColor(row, col) == IS_BACKGROUND) {

                        markAsVisited(row, col);

                        //add pixel to a new blob
                        _currentBlob = new Blob(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col,
                                                IS_BACKGROUND);
                        _currentBlob->addPixel(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col);
                        //make sure we don't look at it again

                        //look at its neighbors
                        if (_rank == 4) {
                            analyseNeighbour4(row, col, IS_BACKGROUND);
                        } else {
                            analyseNeighbour8(row, col, IS_BACKGROUND);
                        }
                    }
                }
            }


            //we look at every pixel in the tile (not the view)
            for (int32_t row = 0; row < _tileHeight; ++row) {
                for (int32_t col = 0; col < _tileWidth; ++col) {


                    //       auto globalRow = row + _view->getGlobalYOffset();
             //       auto globalCol = col + _view->getGlobalXOffset();
//                    VLOG(1) << "pixel in tile (" << row << "," << col << "), view " << "( " << (int32_t) (row - _view->getRadius()) << "," << (int32_t)(col - _view->getRadius()) << ") " << ", global " << "( " << globalRow << "," << globalCol << ")   -> " << _view->getPixel(row,col);


                    //we have created a blob and we are still expanding it.
                    while (!_toVisit.empty()) {
                        auto neighbourCoord = *_toVisit.begin();
                        _toVisit.erase(_toVisit.begin());
                        //mark pixel as visited so we don't look at it again
                        markAsVisited(neighbourCoord.first,
                                        neighbourCoord.second);
                        bool color = getColor(neighbourCoord.first, neighbourCoord.second);

                        _currentBlob->addPixel(
                                _view->getGlobalYOffset() + neighbourCoord.first,
                                _view->getGlobalXOffset() + neighbourCoord.second);


                        if (_rank == 4) {
                            analyseNeighbour4(neighbourCoord.first, neighbourCoord.second, color);
                        } else {
                            analyseNeighbour8(neighbourCoord.first, neighbourCoord.second, color);
                        }
                    }

                    //we have a completed blob, we need to save it.
                    if (_currentBlob != nullptr) {

//                        if (_currentBlob->isBackground() && !_currentBlob->isToMerge() &&
//                            _currentBlob->getCount() < minHoleSize) {
//
//                            Blob *candidate = nullptr;
//
//                            for (auto blob : _vAnalyse->getBlobs()) {
//                                if (blob->isForeground() && blob->isPixelinBoundingBox(_currentBlob->getStartRow(),
//                                                                                       _currentBlob->getStartCol())) {
//                                    if (candidate == nullptr ||
//                                        blob->getBoundingBoxArea() < candidate->getBoundingBoxArea()) {
//                                        candidate = blob;
//                                    }
//                                }
//                            }
//
//                            if (candidate != nullptr) {
//                                VLOG(1) << "found containing object for a hole to remove. Merging";
//                                _vAnalyse->deleteBlob(_currentBlob);
//                                candidate->mergeAndDelete(_currentBlob);
//                            } else {
//                                VLOG(1) << "could not find a containing objec for a hole to remove. Let make it a object.";
//                                auto color = _currentBlob->flipColor();
//                                //look at its neighbors
//                                if (_rank == 4) {
//                                    analyseNeighbour4(_currentBlob->getStartRow() - _view->getGlobalYOffset(),
//                                                      _currentBlob->getStartCol() - _view->getGlobalXOffset(), color);
//                                } else {
//                                    analyseNeighbour8(_currentBlob->getStartRow() - _view->getGlobalYOffset(),
//                                                      _currentBlob->getStartCol() - _view->getGlobalXOffset(), color);
//                                }
//                            }
//                        } else {
                            _vAnalyse->insertBlob(_currentBlob);
                            _previousBlob = _currentBlob;
                            _currentBlob = nullptr;
//                        }
                    }

                    //Find the next blob to create
                    if (!visited(row, col)) {

                        markAsVisited(row, col);

                        //find out the pixel color
                        bool color = getColor(row, col);

                        if(color) {

                            //add pixel to a new blob
                            _currentBlob = new Blob(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col,
                                                    color);
                            _currentBlob->addPixel(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col);
                            //make sure we don't look at it again

                            //look at its neighbors
                            if (_rank == 4) {
                                analyseNeighbour4(row, col, color);
                            } else {
                                analyseNeighbour8(row, col, color);
                            }
                        }
                    }
                }
            }

            // Save the current blob if the BR pixel  is alone and is a blob by itself
            if (_currentBlob != nullptr) {
                _vAnalyse->insertBlob(_currentBlob);
                _currentBlob = nullptr;
                _previousBlob = nullptr;
            }

            _previousBlob = nullptr;
            // Release the view memory
            view->releaseMemory();
            // Add the analyse
            this->addResult(_vAnalyse);
        }

        /// \brief View analyser copy function
        /// \return A new View analyser
        ViewAnalyser<UserType> *copy() override {
            return new ViewAnalyser<UserType>(this->getNumThreads(),
                                              _fi,
                                              _rank,
                                              _background);
        }

        /// \brief Get task name
        /// \return Task name
        std::string getName() override {
            return "View analyser";
        }

    private:
        fi::View<UserType> *
                _view{};                      ///< Current view

        UserType
                _background{};                ///< Pixel background value

        fi::FastImage<UserType> *
                _fi{};                        ///< FI mask

        uint32_t
                _imageHeight{},               ///< Mask height
                _imageWidth{};                ///< Mask width

        uint8_t
                _rank{};                      ///< Rank to the connectivity:
        ///< 4=> 4-connectivity, 8=> 8-connectivity

        int32_t
                _tileHeight{},                ///< Tile actual height
                _tileWidth{};                 ///< Tile actual width

        std::set<Coordinate>
                _toVisit{};                   ///< Set of coordinates to visit (neighbour)

        ViewAnalyse
                *_vAnalyse = nullptr;         ///< Current view analyse

        Blob
                *_currentBlob = nullptr;      ///< Current blob


                //TODO DELETE?
        Blob
                *_previousBlob = nullptr;      ///< Current blob

        std::vector<bool> _visited;


        uint32_t MIN_HOLE_SIZE = 20;
        uint32_t minObjectSize = 20;


    };
}



#endif //EGT_VIEWANALYZER_H
