//
// Created by gerardin on 7/10/19.
//

#include <iostream>

#include <egt/memory/TileAllocator.h>
#include <gtest/gtest.h>
#include <htgs/api/TaskGraphConf.hpp>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/api/EGTOptions.h>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/tasks/FastThresholdFinder.h>

namespace egt {

    class ThresholdFinderGraph {

    public :

        template<class T>
        T init(EGTOptions *options) {

            const uint32_t pyramidLevelToRequestforThreshold = options->pyramidLevel;
            const uint32_t radiusForThreshold = 1;
            const uint32_t startRowConvolution = 1; //where do we start the convolution. Depends on the kernel size. SHould always be < to kernelSize / 2
            const uint32_t startColConvolution = 1; //same
            const uint32_t fastThresholdNumThreads = 1; //always because we collect all states before doing sth smart

            T threshold = 0;

            auto tileLoader = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");

            uint32_t tileWidth = fi->getTileWidth();
            uint32_t tileHeight = fi->getTileHeight();
            uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold) *
                               fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);
            uint32_t tileSize = tileHeight * tileWidth;

            auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
            auto sobelFilter = new CustomSobelFilter3by3<T>(options->concurrentTiles, options->imageDepth, startRowConvolution, startColConvolution);
            auto thresholdFinder = new FastThresholdFinder<T>(fastThresholdNumThreads, tileSize, nbTiles, options->imageDepth);

            graph->addEdge(fastImage, sobelFilter);
            graph->addEdge(sobelFilter, thresholdFinder);
            graph->addGraphProducerTask(thresholdFinder);

            //MEMORY MANAGEMENT
            graph->addMemoryManagerEdge("gradientTile", sobelFilter, new TileAllocator<T>(tileWidth, tileHeight),
                                        options->concurrentTiles, htgs::MMType::Dynamic);

            auto *runtime = new htgs::TaskGraphRuntime(graph);
            runtime->executeRuntime();

            fi->requestAllTiles(true,pyramidLevelToRequestforThreshold);
            graph->finishedProducingData();

            std::vector<T> thresholdCandidates = {};

            while (!graph->isOutputTerminated()) {
                auto data = graph->consumeData();
                if (data != nullptr) {
                    thresholdCandidates.push_back(data->getValue());
                }
            }

            EXPECT_EQ(thresholdCandidates.size(), 1);

            return thresholdCandidates[0];
        }
    };

    class ThresholdFinderTestFixture : public ::testing::Test {
    protected:
        ThresholdFinderGraph graph;
    };

    TEST_F(ThresholdFinderTestFixture, ThresholdFinderTestPhase) {
        auto options = new EGTOptions();
        options->pyramidLevel = 0;

        options->nbLoaderThreads = 1;
        options->concurrentTiles = 1;
        options->imageDepth = ImageDepth::_16U;
        options->inputPath = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";
        uint16_t threshold = graph.init<uint16_t>(options);
        EXPECT_EQ(threshold, 109) << "Threshold is 108 in the original EGT implementation but we should obtain 109.";
    }

    TEST_F(ThresholdFinderTestFixture, ThresholdFinderTestStitched) {
        auto options = new EGTOptions();
        options->pyramidLevel = 0;
        options->nbLoaderThreads = 2;
        options->concurrentTiles = 10;
        options->imageDepth = ImageDepth::_16U;
        options->inputPath = "/home/gerardin/Documents/images/dataset2/images/tiled_stitched_c01t020p1_pyramid_1024.ome.tif";
        uint16_t threshold = graph.init<uint16_t>(options);
        EXPECT_EQ(threshold, 109) << "Threshold is 87 in the original EGT implementation but we should obtain 85.";
    }
}
