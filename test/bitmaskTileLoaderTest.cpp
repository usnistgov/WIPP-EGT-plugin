//
// Created by gerardin on 8/16/19.
//



#include <egt/FeatureCollection/Data/Feature.h>
#include <egt/loaders/FeatureBitmaskLoader.h>
#include <glog/logging.h>
#include <FastImage/api/FastImage.h>
#include <egt/utils/Utils.h>

uint32_t* createBitmask(uint8_t* array, uint32_t width, uint32_t height) {

    auto wordPosition = 0;
    auto bitPosition = 0;
    auto bitmaskSize = (uint32_t) ceil(
            (width * height) / 32.);
    auto bitMask = new uint32_t[bitmaskSize]();

    for(auto wordIndex = 0 ; wordIndex < bitmaskSize; wordIndex++){
        bitMask[wordIndex] = 0;
    }

    // For every pixel in the bit mask
    for (uint32_t row = 0; row < height; ++row) {
        for (uint32_t col = 0; col < width; ++col) {
            auto absolutePosition = row * width + col;
            // Test if the pixel is in the current feature
            if (array[absolutePosition] == 1) {
                // Add it to the bit mask
                wordPosition = absolutePosition >> (uint32_t) 5;
                bitPosition =
                        (uint32_t) abs(32 - ((int32_t) absolutePosition
                                             - ((int32_t) (wordPosition << (uint32_t) 5))));
                bitMask[wordPosition] = bitMask[wordPosition]
                                        | ((uint32_t) 1 << (bitPosition - (uint32_t) 1));
            }
        }
    }

    return bitMask;
}


int main() {
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
    array[36] = 1;
    array[35] = 1;
    array[28] = 1;
    array[27] = 1;

    auto bitmask = createBitmask(array, width, height);
    auto bb = egt::BoundingBox(0,0,width,height);
    auto f = egt::Feature(2,bb,bitmask);
    f.printBitMask();


    //TODO the loader needs an empty constructor for Feature??
    auto loader = new egt::FeatureBitmaskLoader<uint8_t>(f, tileSize);

    auto fi = new fi::FastImage<uint8_t>(loader,0);
    fi->getFastImageOptions()->setNumberOfViewParallel(1);
    fi->configureAndRun();

  //  fi->requestAllTiles(true);

    for(auto row = 0; row <= 1; row++ ){
        for(auto col = 0; col <= 1 ; col ++){
            fi->requestTile(row,col,false);
        }
    }
    fi->finishedRequestingTiles();

    while(fi->isGraphProcessingTiles()){
        auto pview = fi->getAvailableViewBlocking();
        if(pview != nullptr) {
            auto view = pview->get();
            auto data = view->getData();

            int dataOut[16];
            for(auto i = 0 ; i < 4; i ++){
                for(auto j = 0; j < 4 ; j ++){
                    dataOut[i * 4 + j] = (int)data[i * 4 + j];
                }
            }

            egt::printArray<int>("tile", dataOut, 4, 4);

            pview->releaseMemory();
        }

    }

    fi->waitForGraphComplete();

    fi->writeGraphDotFile("debug-fi-graph.xdot");

    delete fi;
}

