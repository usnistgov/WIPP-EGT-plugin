//
// Created by gerardin on 7/10/19.
//

#include <iostream>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/ThresholdFinder.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/ViewToConvOutDataConverter.h>

int main() {

    typedef float T; //Fast Image needs to know how to interpret the pixel value so the whole algorithm is templated.

    //We start from a ground truth gradient image from egt-java
     std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/gradients_tiled256_pyramid.tif";

    uint32_t pyramidLevelToRequest = 0;
    auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, 1);
    fi::FastImage<T> *fi = new fi::FastImage<T>(tileLoader, 0); //make sure we ask for the tile data without extra borders.
    fi->getFastImageOptions()->setNumberOfViewParallel(1);
    auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");

    uint32_t
            imageHeight = fi->getImageHeight(pyramidLevelToRequest),     //< Image Height
            imageWidth = fi->getImageWidth(pyramidLevelToRequest);      //< Image Width


    auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, egt::Threshold<T>>();

    //convert fast image views into or own data structure that is consumed by the threshold finder.
    //In the full pipeline, this is the output of the Sobel filter convolution.
    auto converter = new egt::ViewToConvOutDataConverter<T>(1);

    //The task to test
    auto numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequest);
    auto numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequest);
    auto thresholdFinder = new egt::ThresholdFinder<T>(imageWidth, imageHeight , numTileRow, numTileCol);

    graph->addEdge(fastImage,converter);
    graph->addEdge(converter,thresholdFinder);
    graph->addGraphProducerTask(thresholdFinder);

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
