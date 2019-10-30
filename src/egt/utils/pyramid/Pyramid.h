
//
// Created by gerardin on 5/2/19.
//

#ifndef PYRAMIDBUILDING_PYRAMID_H
#define PYRAMIDBUILDING_PYRAMID_H


#include <cstdio>
#include <algorithm>
#include <cmath>
#include <array>
#include <vector>
#include <assert.h>

namespace pb {


    class Pyramid {


    public :

        Pyramid() = default;

        Pyramid(size_t baseWidth, size_t baseHeight, size_t tileSize) : baseWidth(baseWidth), baseHeight(baseHeight),
                                                                        tileSize(tileSize) {
            numTileRow = (uint32_t)(std::ceil( (double)baseHeight / tileSize));
            numTileCol = (uint32_t)(std::ceil( (double)baseWidth / tileSize));

            auto maxDim = std::max(numTileCol, numTileRow);
            numLevel = static_cast<size_t>(ceil(log2(maxDim)) + 1);

            //calculate number of tiles for each level
            size_t
                    levelCol, levelRow;
            levelCol = numTileCol;
            levelRow = numTileRow;
            for (size_t l = 0; l < numLevel; l++) {
                std::array<size_t, 2> gridSize = {(size_t)levelRow, (size_t)levelCol};
                levelGridSizes.push_back(gridSize);
                levelCol = static_cast<size_t>(ceil((double) levelCol / 2));
                levelRow = static_cast<size_t>(ceil((double) levelRow / 2));
            }
        }

        size_t getPyramidWidth(size_t level) const {
            auto width = static_cast<size_t>(ceil((double)baseWidth/ pow(2,level)));
            return width;
        }

        size_t getPyramidHeight(size_t level) const {
            auto height = static_cast<size_t>(ceil((double)baseHeight/ pow(2,level)));
            return height;
        }


        size_t getBaseWidth() const {
            return baseWidth;
        }

        size_t getBaseHeight() const {
            return baseHeight;
        }

        size_t getTileSize() const {
            return tileSize;
        }


        size_t getNumTileCol(size_t level) const {
            assert(level < numLevel);
            return levelGridSizes[level][1];
        }

        size_t getNumTileRow(size_t level) const {
            assert(level < numLevel);
            return levelGridSizes[level][0];
        }

        size_t getNumLevel() const {
            return numLevel;
        }


    private :
        size_t baseWidth;
        size_t baseHeight;
        size_t tileSize;
        size_t numTileCol;
        size_t numTileRow;
        size_t numLevel;
        std::vector<std::array<size_t,2>> levelGridSizes;


    };
}


#endif //PYRAMIDBUILDING_PYRAMID_H
