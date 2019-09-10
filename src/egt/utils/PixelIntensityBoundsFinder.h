//
// Created by gerardin on 9/9/19.
//

#ifndef NEWEGT_PIXELINTENSITYBOUNDSFINDER_H
#define NEWEGT_PIXELINTENSITYBOUNDSFINDER_H

#include <egt/api/EGTOptions.h>
#include <egt/api/SegmentationOptions.h>
#include <egt/api/DerivedSegmentationParams.h>
#include <glog/logging.h>
#include <chrono>
#include <random>

namespace egt {

    template <class T>
    class PixelIntensityBoundsFinder {

    public :

        void runPixelIntensityBounds(EGTOptions *options, SegmentationOptions* segmentationOptions, DerivedSegmentationParams<T> &segmentationParams) {

            auto randomExperiments = true;
            if(options->nbTilePerSample == -1 && options->nbExperiments == -1) {
                randomExperiments = false;
            }

            auto nbOfSamplingExperiment = (size_t)((options->nbExperiments != -1) ? options->nbExperiments : 1);

            if(!randomExperiments) {
                nbOfSamplingExperiment = 1;
            }

            auto minValue = std::numeric_limits<T>::max();
            auto maxValue = std::numeric_limits<T>::min();

            auto expCount = 0;

            while(expCount < nbOfSamplingExperiment) {

                const uint32_t radiusForThreshold = 0;

                auto tileLoader = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
                auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
                fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
                fi->configureAndRun();
                //we try to figure at which resolution we could calculate the intensity.
                uint32_t pyramidLevelToRequestforPixelIntensityBounds = options->pyramidLevel;
                auto levelUp = options->pixelIntensityBoundsLevelUp;
                while (pyramidLevelToRequestforPixelIntensityBounds + levelUp > fi->getNbPyramidLevels() - 1){
                    levelUp--;
                }
                pyramidLevelToRequestforPixelIntensityBounds += levelUp;
                uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforPixelIntensityBounds);
                uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforPixelIntensityBounds);
                uint32_t numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforPixelIntensityBounds);
                uint32_t numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforPixelIntensityBounds);
                uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestforPixelIntensityBounds) *
                                   fi->getNumberTilesWidth(pyramidLevelToRequestforPixelIntensityBounds);

                auto nbOfSamples = (options->nbTilePerSample != -1) ? std::min((int32_t)nbTiles, options->nbTilePerSample) : nbTiles;
                VLOG(1) << "Pixel intensity bounds finder. Nb of tiles per sample: " << nbOfSamples << std::endl;
                VLOG(1) << "Pixel intensity bounds finder. Nb of experiments: " << nbOfSamplingExperiment << std::endl;

                auto intensities = std::vector<T>();

                if(randomExperiments) {
                    auto seed1 = std::chrono::system_clock::now().time_since_epoch().count();
                    std::default_random_engine generator(seed1);
                    std::uniform_int_distribution<uint32_t> distributionRowRange(0, numTileRow - 1);
                    std::uniform_int_distribution<uint32_t> distributionColRange(0, numTileCol - 1);
                    for (auto i = 0; i < nbOfSamples; i++) {
                        uint32_t randomRow = distributionRowRange(generator);
                        uint32_t randomCol = distributionColRange(generator);
                        VLOG(3) << "Requesting tile : (" << randomRow << ", " << randomCol << ")";
                        fi->requestTile(randomRow, randomCol, false, pyramidLevelToRequestforPixelIntensityBounds);
                    }
                    fi->finishedRequestingTiles();
                } else {
                    fi->requestAllTiles(true,pyramidLevelToRequestforPixelIntensityBounds);
                }

                while(fi->isGraphProcessingTiles()) {
                    auto pview = fi->getAvailableViewBlocking();
                    if(pview != nullptr){
                        auto view = pview->get();
                        auto tileHeight = view->getTileHeight();
                        auto tileWidth = view->getTileWidth();
                        VLOG(3) << "intensity bounds : collecting pixel for tile (" << view->getRow() << "," << view->getCol() << ").";
                        //TODO check with it has to be zero
                        std::copy_if(view->getData(), view->getData() + tileHeight * tileWidth,  std::back_inserter(intensities) , [](T val){return (val!=0);});
                        pview->releaseMemory();
                    }
                }

                auto nonZeroSize = intensities.size();

                std::sort(intensities.begin(), intensities.end());

                auto minIndex = (double)(segmentationOptions->MIN_PIXEL_INTENSITY_PERCENTILE * intensities.size()) / 100;
                auto maxIndex = (double)(segmentationOptions->MAX_PIXEL_INTENSITY_PERCENTILE * intensities.size()) / 100  - 1;

                minValue = std::min( minValue, intensities[minIndex]);
                maxValue = std::max( maxValue, intensities[maxIndex]);

                expCount++;

                fi->waitForGraphComplete();
                delete fi;
            }

            segmentationParams.minPixelIntensityValue = minValue;
            segmentationParams.maxPixelIntensityValue = maxValue;

            VLOG(1) << "min pixel intensity : " << segmentationParams.minPixelIntensityValue;
            VLOG(1) << "max pixel intensity : " << segmentationParams.maxPixelIntensityValue;

            }


    void runFastPixelIntensityBounds(EGTOptions *options, SegmentationOptions* segmentationOptions, DerivedSegmentationParams<T> &segmentationParams) {

        auto randomExperiments = true;
        if(options->nbTilePerSample == -1 && options->nbExperiments == -1) {
            randomExperiments = false;
        }

        auto nbOfSamplingExperiment = (size_t)((options->nbExperiments != -1) ? options->nbExperiments : 1);

        if(!randomExperiments) {
            nbOfSamplingExperiment = 1;
        }

        auto minIntensity = minValue;
        auto maxIntensity = maxValue;



        auto expCount = 0;

        while(expCount < nbOfSamplingExperiment) {

            const uint32_t radiusForThreshold = 0;

            auto tileLoader = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            fi->configureAndRun();
            //we try to figure at which resolution we could calculate the intensity.
            uint32_t pyramidLevelToRequestforPixelIntensityBounds = options->pyramidLevel;
            auto levelUp = options->pixelIntensityBoundsLevelUp;
            while (pyramidLevelToRequestforPixelIntensityBounds + levelUp > fi->getNbPyramidLevels() - 1){
                levelUp--;
            }
            pyramidLevelToRequestforPixelIntensityBounds += levelUp;
            uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforPixelIntensityBounds);
            uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforPixelIntensityBounds);
            uint32_t numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforPixelIntensityBounds);
            uint32_t numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforPixelIntensityBounds);
            uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestforPixelIntensityBounds) *
                               fi->getNumberTilesWidth(pyramidLevelToRequestforPixelIntensityBounds);

            auto nbOfSamples = (options->nbTilePerSample != -1) ? std::min((int32_t)nbTiles, options->nbTilePerSample) : nbTiles;
            VLOG(1) << "Pixel intensity bounds finder. Nb of tiles per sample: " << nbOfSamples << std::endl;
            VLOG(1) << "Pixel intensity bounds finder. Nb of experiments: " << nbOfSamplingExperiment << std::endl;

            //TODO size should not be hardcoded
            auto intensities = std::vector<T>(256 * 256);

            if(randomExperiments) {
                auto seed1 = std::chrono::system_clock::now().time_since_epoch().count();
                std::default_random_engine generator(seed1);
                std::uniform_int_distribution<uint32_t> distributionRowRange(0, numTileRow - 1);
                std::uniform_int_distribution<uint32_t> distributionColRange(0, numTileCol - 1);
                for (auto i = 0; i < nbOfSamples; i++) {
                    uint32_t randomRow = distributionRowRange(generator);
                    uint32_t randomCol = distributionColRange(generator);
                    VLOG(3) << "Requesting tile : (" << randomRow << ", " << randomCol << ")";
                    fi->requestTile(randomRow, randomCol, false, pyramidLevelToRequestforPixelIntensityBounds);
                }
                fi->finishedRequestingTiles();
            } else {
                fi->requestAllTiles(true,pyramidLevelToRequestforPixelIntensityBounds);
            }

            while(fi->isGraphProcessingTiles()) {
                auto pview = fi->getAvailableViewBlocking();
                if(pview != nullptr){
                    auto view = pview->get();
                    auto tileHeight = view->getTileHeight();
                    auto tileWidth = view->getTileWidth();
                    VLOG(3) << "intensity bounds : collecting pixel for tile (" << view->getRow() << "," << view->getCol() << ").";
                    binSample(view, intensities);
                    pview->releaseMemory();
                }
            }

            const double minVal = segmentationOptions->MIN_PIXEL_INTENSITY_PERCENTILE * pixelCount / 100;
            const double maxVal = segmentationOptions->MAX_PIXEL_INTENSITY_PERCENTILE * pixelCount / 100;

            const T lowerBound = minValue;
            const T higherBound = maxValue;

            VLOG(3) << "Done. Finding lower and higher bounds between " << (double)lowerBound << " and "<< (double)higherBound << "...";

            auto pixelCount = 0;
            for(auto k = lowerBound; k <= higherBound; k++){
                if(pixelCount > minVal) {
                    minValue = k - 1;
                    break;
                }
                pixelCount += intensities[k];
            }

            VLOG(3) << "min intensity : " << (double)minValue;

            pixelCount = 0;
            for(auto l = lowerBound; l <= higherBound; l++){
                if(pixelCount < maxVal) {
                    maxValue = l;
                }
                pixelCount += intensities[l];
            }

            VLOG(3) << "max intensity : " << (double)maxValue;

            VLOG(3) << "Done.";

            expCount++;

            fi->waitForGraphComplete();
            delete fi;
        }

        VLOG(3) << "Done with all sample experiments.";

        segmentationParams.minPixelIntensityValue = minValue;
        segmentationParams.maxPixelIntensityValue = maxValue;

        VLOG(1) << "min pixel intensity : " << (double)segmentationParams.minPixelIntensityValue;
        VLOG(1) << "max pixel intensity : " << (double)segmentationParams.maxPixelIntensityValue;

    }

    void binSample(fi::View<T> *data, std::vector<T> &hist){
        auto sampleSize = data->getTileWidth() * data->getTileHeight();
        for(auto row = 0; row < data->getTileHeight(); row++){
            for(auto col =0; col < data->getTileWidth(); col++){
                auto index = data->getPixel(row,col);
                hist[index]++;
                if(index !=0) {
                    if (index < minValue) {
                        minValue = index;
                    }
                    if (index > maxValue) {
                        maxValue = index;
                    }
                    pixelCount++;
                }
            }
        }
    }

    private:
        double pixelCount = 0;
        T minValue = std::numeric_limits<T>::max();
        T maxValue = std::numeric_limits<T>::min();
};

}


#endif //NEWEGT_PIXELINTENSITYBOUNDSFINDER_H
