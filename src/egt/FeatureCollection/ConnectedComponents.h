//
// Created by gerardin on 7/19/19.
//

#ifndef NEWEGT_CONNECTEDCOMPONENTS_H
#define NEWEGT_CONNECTEDCOMPONENTS_H

#include <cstdint>
#include <unordered_set>
#include<egt/FeatureCollection/Data/Blob.h>
#include <egt/utils/Utils.h>
#include <egt/FeatureCollection/Tasks/SegmentationOptions.h>
#include <egt/FeatureCollection/Data/ViewAnalyse.h>

using namespace fc;

namespace egt {

    template<class UserType>
    class ConnectedComponents {

    public:

        enum Color {FOREGROUND, BACKGROUND };


        class Options {

        public:

            Options(const Color blobColor, const uint8_t rank, const UserType &background) : blobColor(blobColor),
                                                                                             rank(rank),
                                                                                             background(background) {}

            const Color blobColor = Color::BACKGROUND;
            const uint8_t rank = 4;
            const UserType background{};

            uint32_t MIN_HOLE_SIZE{};
            uint32_t MAX_HOLE_SIZE{};
            uint32_t MIN_OBJECT_SIZE{};

            bool MASK_ONLY = false;


        };

        ConnectedComponents(fi::View<UserType>* view,
                const uint32_t imageHeight,
                const uint32_t imageWidth,
                ConnectedComponents::Options options
                )
                : _view(view), _imageHeight(imageHeight), _imageWidth(imageWidth), _options(options){
            _tileWidth = view.getTileWidth();
            _tileHeight = view.getTileHeight();

            //set cutoff values in maximum acceptable range if not set
            if(_options->MAX_HOLE_SIZE <= 0 || _options->MAX_HOLE_SIZE > _tileWidth * _tileHeight){
                options->maxBlobSize = _tileWidth * _tileHeight;
            }
            if(options->minBlobSize > _tileWidth * _tileHeight){
                options.MIN_HOLE_SIZE = 0;
            }

            _visited = std::vector<bool>((unsigned long)(_tileWidth * _tileHeight), false);

            _vAnalyse(nullptr); //collect blobs that needs merge
        }


        void run(){
            for (int32_t row = 0; row < _tileHeight; ++row) {
                for (int32_t col = 0; col < _tileWidth; ++col) {

                    //WE ENTER HERE ONCE WE HAVE CREATED A BLOB THAT HAS NEIGHBORS OF THE SAME COLOR
                    while (!_toVisit.empty()) {
                        expandBlob();
                    }

                    //WE ENTER HERE WHEN WE ARE DONE EXPANDING THE CURRENT BLOB
                    if (_currentBlob != nullptr) {
                        bloblCompleted();
                    }

                    //UNLESS WE ARE DONE EXPLORING WE CREATE A ONE PIXEL BLOB AND EXPLORE ITS NEIGHBORS
                    if (!visited(row, col) && getColor(row, col) == _options->blobColor) {
                        createBlob(row,col);
                    }
                }
            }


       }


    private:


        void expandBlob(){
            auto neighbourCoord = *_toVisit.begin();
            _toVisit.erase(_toVisit.begin());
            //mark pixel as visited so we don't look at it again
            markAsVisited(neighbourCoord.first,
                          neighbourCoord.second);

            _currentBlob->addPixel(
                    _view->getGlobalYOffset() + neighbourCoord.first,
                    _view->getGlobalXOffset() + neighbourCoord.second);


            if (_rank == 4) {
                analyseNeighbour4(neighbourCoord.first, neighbourCoord.second, _options->blobColor, true);
            }
        }

        void bloblCompleted(){
                VLOG(5) << "blob size: " << _currentBlob->getCount();

                //background and foreground blobs are not handled the same way
                if(_options->blobColor == BACKGROUND) {

                    //IF WE HAVE A SMALL BLOB THAT IS NOT TO BE MERGED, WE CONSIDER IT IS A HOLE THAT NEEDS TO BE FILLED
                    //we reset all pixels as UNVISITED foreground.
                    if (_currentBlob->getCount() < _options->MIN_HOLE_SIZE && !_currentBlob->isToMerge()) {

                        for (auto it = _currentBlob->getRowCols().begin();
                             it != _currentBlob->getRowCols().end(); ++it) {
                            auto pRow = it->first;
                            for (auto pCol : it->second) {
                                auto xOffset = _view->getGlobalXOffset();
                                auto yOffset = _view->getGlobalYOffset();
                                markAsUnvisited(pRow - yOffset, pCol - xOffset);
                                _view->setPixel(pRow - yOffset, pCol - xOffset, _background + 1);
                            }
                        }
                        delete _currentBlob;
                        _currentBlob = nullptr;
                        holeRemovedCount++;
                    }
                    else if(_currentBlob->getCount() > _options->MAX_HOLE_SIZE) {
                        //TODO remove from the merge list since we know it is background
                        delete _currentBlob;
                        _currentBlob = nullptr;
                        backgroundCount++;
                    }
                        //we do not know enough, so we need to merge the background blob
                    else {
                        //TODO implement
                        //insert blob to list of holes in vAnalyse
                    }

                }
                else {
                    //WE HAVE A SMALL OBJECT THAT DOES NOT NEED MERGE, WE REMOVE IT
                    if (_currentBlob->getCount() < _options->MIN_OBJECT_SIZE && !_currentBlob->isToMerge()) {
                        delete _currentBlob;
                        _currentBlob = nullptr;
                        objectRemovedCount++;
                    }
                    else{
                        //WE ADD IT
                        _vAnalyse->insertBlob(_currentBlob);
                        _currentBlob = nullptr;
                    }
                }
        }


        void createBlob(int32_t row, int32_t col){
            markAsVisited(row, col);
            //add pixel to a new blob
            _currentBlob = new Blob(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col);
            _currentBlob->addPixel(_view->getGlobalYOffset() + row, _view->getGlobalXOffset() + col);
            //make sure we don't look at it again

            //look at its neighbors
            if (_rank == 4) {
                analyseNeighbour4(row, col, _options->blobColor, true);
            }
        }


        //TODO put back setToMerge (we need to make decision about removing them or not)

        /// \brief Analyse the neighbour of a pixel for a 4-connectivity
        /// \param row Pixel's row
        /// \param col Pixel's col
        void analyseNeighbour4(int32_t row, int32_t col, Color color,bool erode) {

            auto neighborOfDifferentColorCount = 0;

            //Explore neighbors in all directions to see if they need to be added to the blob.
            //Top Pixel
            if (row >= 1) {
                if (getColor(row - 1, col) == color) {
                    _toVisit.emplace(row - 1, col);
                }
                else {
                    neighborOfDifferentColorCount++;
                }
            }
            // Bottom pixel
            if (row + 1 < _tileHeight) {
                if (getColor(row + 1, col) = color) {
                    _toVisit.emplace(row + 1, col);
                }
                else {
                    neighborOfDifferentColorCount++;
                }
            }
            // Left pixel
            if (col >= 1) {
                if (getColor(row, col - 1) == color) {
                    _toVisit.emplace(row, col - 1);
                }
                else {
                    neighborOfDifferentColorCount++;
                }
            }
            // Right pixel
            if ( (col + 1) < _tileWidth) {
                if (getColor(row, col + 1) == color) {
                    _toVisit.emplace(row, col + 1);
                }
                else {
                    neighborOfDifferentColorCount++;
                }
            }


            if(_options->MASK_ONLY){
                return;
            }
            //Check if the blob in this tile belongs to a bigger blob extending several tiles.
            //We only to look at EAST and SOUTH so we can later merge only once, TOP to BOTTOM and LEFT to RIGHT.


            // Add blob to merge list if tile bottom pixel has the same value than the view pixel below (continuity)
            // test that we are also not at the edge of the full image.
            if (row + 1 == _tileHeight && row + _view->getGlobalYOffset() + 1 != _imageHeight) {
                if (getColor(row, col + 1) == color) {
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
                if (getColor(row, col + 1) == color) {
                    _vAnalyse->addToMerge(_currentBlob,
                                          std::pair<int32_t, int32_t>(
                                                  row + _view->getGlobalYOffset(),
                                                  col + 1 + _view->getGlobalXOffset()));
                    _currentBlob->setToMerge(true);
                }
            }

            //look at the pixel on the left
            if (col == 0 && col + _view->getGlobalXOffset() != 0) {
                if (getColor(row, col + 1) == color) {
                    _currentBlob->setToMerge(true);
                }
            }

            //look at the pixel above
            if (row == 0 && row + _view->getGlobalYOffset() != 0) {
                if (getColor(row, col + 1) == color) {
                    _currentBlob->setToMerge(true);
                }
            }
        }


        void markAsVisited(int32_t row, int32_t col) {
            _visited[row * _tileWidth + col] = true;
        }

        void markAsUnvisited(int32_t row, int32_t col) {
            _visited[row * _tileWidth + col] = false;
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

        ///attributes

        fi::View<UserType>* _view;

        uint8_t _rank{};                      ///< Rank to the connectivity:
        ///< 4=> 4-connectivity, 8=> 8-connectivity


        std::vector<bool> _visited{}; ///keep track of every pixel we looked at.
        std::set<Coordinate>_toVisit{}; /// for a current position, keep track of the neighbors that needs visit.


        Blob
                *_currentBlob = nullptr;      ///< Current blob

        Color
                _currentColor = BACKGROUND;

        int32_t
                _tileHeight{},                ///< Tile actual height
                _tileWidth{};                 ///< Tile actual width


        UserType _background{}; /// Threshold value chosen to represent the value above which we have a foreground pixel.

        ///We need those if we are trying to merge blobs globally.
        uint32_t
                _imageHeight{},               ///< Mask height
                _imageWidth{};                ///< Mask width

        ConnectedComponents::Options* _options{};


        ViewAnalyse *_vAnalyse = nullptr;         ///< Current view analyse

        uint32_t objectRemovedCount{},
                 holeRemovedCount{},
                 backgroundCount{};


    };


}

#endif //NEWEGT_CONNECTEDCOMPONENTS_H
