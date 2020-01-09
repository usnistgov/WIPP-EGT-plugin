//
// Created by gerardin on 8/16/19.
//


#include <FastImage/api/FastImage.h>
#include "../../src/egt/utils/Bitmask.h"
#include "../../src/egt/FeatureCollection/Data/BoundingBox.h"
#include "../../src/egt/FeatureCollection/Data/Feature.h"
#include "../../src/egt/loaders/FeatureBitmaskLoader.h"
#include "gtest/gtest.h"

/**
 * Validates that the BitmaskTileLoader can :
 * - take a bitmask as input
 * - provide arrays that represents tiles of this bitmask
 */
TEST(FeatureCollection, BitmaskTileLoader)
    {

    uint8_t foregroundValue = 1;

    //Create a 8 x 8 array with the center four pixels set to foreground.
    //We will use it to create our bitmask and as ground truth to check the loader work s properly.
    uint32_t width = 8;
    uint32_t height = 8;
    size_t size = width * height;
    auto tileSize = width / 2;

    uint8_t array[size];
    for(auto i =0 ; i < width; i ++){
        for (auto j = 0; j < height; j++){
                array[i * width + j ] = 0;
        }
    }

    array[36] = foregroundValue;
    array[35] = foregroundValue;
    array[28] = foregroundValue;
    array[27] = foregroundValue;

    //Turn into a bitmask
    auto bitmask = egt::BitmaskAlgorithms::arrayToBitMask<uint8_t>(array, width, height, foregroundValue);
    auto bb = egt::BoundingBox(0,0,width,height);
    auto f = egt::Feature(2,bb,bitmask);

    VLOG(3) << f.printBitmask();

    //load the bitmask tile by tile
    auto loader = new egt::FeatureBitmaskLoader<uint8_t>(f, tileSize, foregroundValue);

    auto fi = new fi::FastImage<uint8_t>(loader,0);
    fi->getFastImageOptions()->setNumberOfViewParallel(1);
    fi->configureAndRun();
    fi->requestAllTiles(true);

    while(fi->isGraphProcessingTiles()){
        auto pview = fi->getAvailableViewBlocking();
        if(pview != nullptr) {
            auto view = pview->get();
            auto data = view->getData();

            auto xOffset = view->getGlobalXOffset();
            auto yOffset = view->getGlobalYOffset();

            int dataOut[tileSize * tileSize];
            for(auto i = 0 ; i < tileSize; i ++){
                for(auto j = 0; j < tileSize ; j ++){
                    dataOut[i * tileSize + j] = (int)data[i * tileSize + j];

                    //compare to our ground truth
                    EXPECT_EQ(dataOut[i * tileSize + j], array[(i + yOffset) * width + xOffset + j] == foregroundValue);
                }
            }

            VLOG(3) << egt::printArray<int>("tile", dataOut, tileSize, tileSize);

            pview->releaseMemory();
        }

    }

    fi->waitForGraphComplete();
    fi->writeGraphDotFile("debug-fi-graph.xdot");

    delete fi;

    delete[] bitmask;
}

