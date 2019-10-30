//
// Created by gerardin on 4/30/19.
//

#ifndef PYRAMIDBUILDING_RECURSIVEBLOCKTRAVERSAL_H
#define PYRAMIDBUILDING_RECURSIVEBLOCKTRAVERSAL_H

#include <cstdint>
#include <glog/logging.h>
#include <cmath>
#include <assert.h>
#include "Pyramid.h"

namespace pb {

    class RecursiveBlockTraversal {

    public:
        RecursiveBlockTraversal(Pyramid &pyramid) : pyramid(pyramid) {
            blockTraversal(0,0, pyramid.getNumLevel()-1);
        }


        bool isValid(size_t row,size_t col, size_t level){
            return ( row < pyramid.getNumTileRow(level) && col < pyramid.getNumTileCol(level));

        }

        void blockTraversal(size_t row, size_t col, size_t level) {

            //deal with matrices not square.
            if(!isValid(row, col, level)){
                return;
            }

            if(level == 0){
                traversal.emplace_back(row,col);
                return;
            }

            blockTraversal(2 * row, 2 * col, level - 1);
            blockTraversal(2 * row, 2 * col + 1, level - 1);
            blockTraversal(2 * row + 1, 2 * col, level - 1);
            blockTraversal(2 * row + 1, 2 * col + 1, level - 1);

        }


        const std::vector<std::pair<uint32_t, int32_t>> &getTraversal() const {
            return traversal;
        }

    private:

        Pyramid pyramid;
        std::vector<std::pair<uint32_t ,int32_t>> traversal;


    };


}



#endif //PYRAMIDBUILDING_RECURSIVEBLOCKTRAVERSAL_H
