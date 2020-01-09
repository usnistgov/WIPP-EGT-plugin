//
// Created by gerardin on 7/10/19.
//

#include <iostream>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/FeatureCollection/Tasks/ViewAnalyser.h>
#include <egt/FeatureCollection/Tasks/BlobMerger.h>
#include <egt/FeatureCollection/Tasks/FeatureCollection.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/ThresholdFinder.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>

int main() {



    typedef uint16_t T;

    fi::FastImage<T> *fi;
    uint32_t
            imageHeight,     //< Image Height
            imageWidth;      //< Image Width
    uint32_t
            tileWidth;

    htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs> *analyseGraph;  //< Analyse graph
    htgs::TaskGraphRuntime *analyseRuntime; //< Analyse runtime

     std::string path = "/home/gerardin/Documents/images/egt-test-images/egt_test/fillHoles_tiled256_pyramid.tif"; //threshold = 0
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/datasetSegmentationTest2/test2_160px_tiled64_8bit.tif"; //threshold =100
//    std::string path = "/home/gerardin/Documents/images/egt-test-images/dataset01/images/test01-tiled.tif";


//TODO for now we bypass the threshold selection mechanism and work directly with masks
    auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, 1);
    fi = new fi::FastImage<T>(tileLoader, 1);
    fi->getFastImageOptions()->setNumberOfViewParallel(1);
    fi->configureAndRun();
    imageHeight = fi->getImageHeight();
    imageWidth = fi->getImageWidth();
    tileWidth = fi->getTileWidth();

    analyseGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs>;

    //TODO set the right threshold for the image
    auto viewAnalyseTask = new egt::ConvOutDataAnalyser<T>(1, fi, 4, 0);
    auto fileCreation = new BlobMerger(imageHeight, imageWidth, fi->getNumberTilesHeight() * fi->getNumberTilesWidth());

    analyseGraph->setGraphConsumerTask(viewAnalyseTask);
    analyseGraph->addEdge(viewAnalyseTask, fileCreation);
    analyseGraph->addGraphProducerTask(fileCreation);
    analyseRuntime = new htgs::TaskGraphRuntime(analyseGraph);
    analyseRuntime->executeRuntime();

    try {
//Request all tiles from the FC

        VLOG(1) << "request all tiles...";
        fi->requestAllTiles(true);
        while (fi->isGraphProcessingTiles()) {
            auto pView = fi->getAvailableViewBlocking();
            if (pView != nullptr) {
//Feed the graph with the views
                analyseGraph->produceData(pView);
            }
        }
    } catch (const fi::FastImageException &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    analyseGraph->finishedProducingData();

    auto blob = analyseGraph->consumeData();

    auto listblob = blob.get();

    auto originalNbOfBlobs = blob->_blobs.size();
    VLOG(1) << "original nb of blobs : " << originalNbOfBlobs;


    egt::FeatureCollection *fc = new egt::FeatureCollection();
    fc->createFCFromListBlobs(listblob, imageHeight, imageWidth);
    fc->createBlackWhiteMask("output.tiff", tileWidth);


// Wait for the analyse graph to finish processing tiles to make the FC
// available
    analyseRuntime->waitForRuntime();
    fi->waitForGraphComplete();

    analyseGraph->writeDotToFile("FeatureCollectionGraph.xdot", DOTGEN_COLOR_COMP_TIME);

// Delete HTGS graphs
    delete (fi);
    delete (analyseRuntime);

}