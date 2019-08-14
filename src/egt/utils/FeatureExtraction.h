//
// Created by gerardin on 8/8/19.
//

#ifndef NEWEGT_COMPUTEMEANINTENSITY_H
#define NEWEGT_COMPUTEMEANINTENSITY_H

#include <cstdint>
#include <egt/api/EGTOptions.h>
#include <egt/FeatureCollection/Data/Blob.h>
#include <egt/loaders/PyramidTiledTiffLoader.h>

namespace egt {

    template <class T>
    void computeMeanIntensity(std::list<Blob*> blobs, EGTOptions *options, std::unordered_map<Blob*,T>* meanIntensities){


        const uint32_t pyramidLevelToRequestforThreshold = options->pyramidLevel;
        const uint32_t radiusForFeatureExtraction = 0;

        auto tileLoader = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
        auto *fi = new fi::FastImage<T>(tileLoader, radiusForFeatureExtraction);
        fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
        fi->configureAndRun();

        uint32_t
                indexRowMin = 0,
                indexRowMax = 0,
                indexColMin = 0,
                indexColMax = 0;

        uint32_t
                minRow = 0,
                maxRow = 0,
                minCol = 0,
                maxCol = 0;

        for (auto blob : blobs) {
            T pixValue = 0,
                    featureMeanIntensity = 0;
            uint64_t count = 0;
            uint64_t sum = 0;
            uint32_t featureTotalTileCount = 0;

            auto feature = blob->getFeature();

            //We cannot use fc::feature because we reimplemented it in the egt namespace
            //let's request all tiles for each feature
            const auto &bb = feature->getBoundingBox();
            indexRowMin = bb.getUpperLeftRow() / fi->getTileHeight();
            indexColMin = bb.getUpperLeftCol() / fi->getImageWidth();
            // Handle border case
            if (bb.getBottomRightCol() == fi->getImageWidth())
                indexColMax = fi->getNumberTilesWidth();
            else
                indexColMax = (bb.getBottomRightCol() / fi->getTileWidth()) + 1;
            if (bb.getBottomRightRow() == fi->getImageHeight())
                indexRowMax = fi->getNumberTilesHeight();
            else
                indexRowMax = (bb.getBottomRightRow() / fi->getTileHeight()) + 1;
            for (auto indexRow = indexRowMin; indexRow < indexRowMax; ++indexRow) {
                for (auto indexCol = indexColMin; indexCol < indexColMax; ++indexCol) {
                    fi->requestTile(indexRow,indexCol, false);
                    featureTotalTileCount++;
                }
            }

            auto tileCount = 0;

            while (tileCount < featureTotalTileCount) {
                auto view = fi->getAvailableViewBlocking();
                if (view != nullptr) {
                    minRow = std::max(view->get()->getGlobalYOffset(),
                                      feature->getBoundingBox().getUpperLeftRow());
                    maxRow = std::min(
                            view->get()->getGlobalYOffset() + view->get()->getTileHeight(),
                            feature->getBoundingBox().getBottomRightRow());
                    minCol = std::max(view->get()->getGlobalXOffset(),
                                      feature->getBoundingBox().getUpperLeftCol());
                    maxCol = std::min(
                            view->get()->getGlobalXOffset() + view->get()->getTileWidth(),
                            feature->getBoundingBox().getBottomRightCol());

                    for (auto row = minRow; row < maxRow; ++row) {
                        for (auto col = minCol; col < maxCol; ++col) {
                            if (feature->isImagePixelInBitMask(row, col)) {
                                pixValue = view->get()->getPixel(
                                        row - view->get()->getGlobalYOffset(),
                                        col - view->get()->getGlobalXOffset());
                                sum += pixValue;
                                count++;
                            }
                        }
                    }
                    view->releaseMemory();
                    tileCount ++;
                }
            }

            assert(count != 0);
            featureMeanIntensity = std::round(sum / count);
            meanIntensities->insert({blob, featureMeanIntensity});
        }

        fi->finishedRequestingTiles();
        fi->waitForGraphComplete();
        delete fi;
    }
}

#endif //NEWEGT_COMPUTEMEANINTENSITY_H
