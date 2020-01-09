//
// Created by gerardin on 7/10/19.
//

#include <iostream>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/ThresholdFinder.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/memory/TileAllocator.h>
#include <>

int main() {
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";
//     std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/inputs/phase_image_002_tiled256.tif";
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/datasetSegmentationTest2/test2_160px_tiled64_8bit.tif";
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/dataset01/images/test01-tiled.tif";
    std::string path = "/Users/gerardin/Documents/images/egt_test/inputs/phase_image_002_tiled256_pyramid.tif";

    typedef  float T;

    ImageDepth depth = ImageDepth::_32F;
    uint32_t pyramidLevelToRequest = 0;

    auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, 1);
    fi::FastImage<T> *fi = new fi::FastImage<T>(tileLoader, 1);
    fi->getFastImageOptions()->setNumberOfViewParallel(1);
    auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");

    uint32_t
            imageHeight = fi->getImageHeight(pyramidLevelToRequest),     //< Image Height
            imageWidth = fi->getImageWidth(pyramidLevelToRequest);      //< Image Width


    auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, egt::Threshold<T>>();

    auto concurrentTiles = 1;

    auto sobelFilter = new egt::CustomSobelFilter3by3<T>(concurrentTiles, depth);
//    auto sobelFilter = new egt::SobelFilterOpenCV<T>(1, depth);

    auto numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequest);
    auto numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequest);
    auto thresholdFinder = new egt::ThresholdFinder<T>(imageWidth, imageHeight , numTileRow, numTileCol);

    graph->addEdge(fastImage,sobelFilter);
    graph->addEdge(sobelFilter,thresholdFinder);
    graph->addGraphProducerTask(thresholdFinder);

    //MEMORY MANAGEMENT
    auto tileWidth = fi->getTileWidth();
    graph->addMemoryManagerEdge("gradientTile",sobelFilter, new egt::TileAllocator<T>(tileWidth , tileWidth),concurrentTiles, htgs::MMType::Dynamic);

    htgs::TaskGraphSignalHandler::registerTaskGraph(graph);
    htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

    auto *runtime = new htgs::TaskGraphRuntime(graph);
    runtime->executeRuntime();

    fi->requestAllTiles(true,pyramidLevelToRequest);
    graph->finishedProducingData();

    T threshold = 0;

    while (!graph->isOutputTerminated()) {
        auto data = graph->consumeData();

        if (data != nullptr) {
            threshold = data->getValue();
            VLOG(1) << "Threshold value : " << threshold;
        }
    }

    runtime->waitForRuntime();

    graph->writeDotToFile("colorGraph.xdot", DOTGEN_COLOR_COMP_TIME);

    delete runtime;

    return 0;
}
