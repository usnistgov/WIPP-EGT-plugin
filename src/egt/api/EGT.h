//
// Created by gerardin on 7/9/19.
//

#ifndef NEWEGT_EGT_H
#define NEWEGT_EGT_H


#include <string>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/FeatureCollection/Tasks/ViewAnalyser.h>
#include <egt/FeatureCollection/Tasks/BlobMerger.h>
#include <egt/FeatureCollection/Tasks/FeatureCollection.h>
#include "DataTypes.h"
#include <egt/tasks/ThresholdFinder.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/FCSobelFilterOpenCV.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/memory/TileAllocator.h>


namespace egt {

    template<class T>
    class EGT {

    private:

        uint32_t _minObjectSize = 100;

    public:


        void run(std::string path) {


            auto begin = std::chrono::high_resolution_clock::now();

            ImageDepth depth = ImageDepth::_32F;
            uint32_t pyramidLevelToRequestforThreshold = 0;
            uint32_t concurrentTiles = 10;

            auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, 2);
            auto *fi = new fi::FastImage<T>(tileLoader, 1);
            fi->getFastImageOptions()->setNumberOfViewParallel(concurrentTiles);
            auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");


            uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforThreshold);     //< Image Height
            uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforThreshold);      //< Image Width

            uint32_t tileWidth = fi->getTileWidth();
            uint32_t tileHeight = fi->getTileHeight();
            assert(tileWidth == tileHeight); //we work with square tiles

            auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
            auto sobelFilter = new CustomSobelFilter3by3<T>(10, depth);
            auto numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);
            auto numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold);
            auto thresholdFinder = new egt::ThresholdFinder<T>(imageWidth, imageHeight , numTileRow, numTileCol);

            graph->addEdge(fastImage,sobelFilter);
            graph->addEdge(sobelFilter,thresholdFinder);
            graph->addGraphProducerTask(thresholdFinder);

            //MEMORY MANAGEMENT
            //TODO CHECK that memoryPoolSize is it the number of tile data of size tileWidth * tileHeight we can allocate
            graph->addMemoryManagerEdge("gradientTile",sobelFilter, new TileAllocator<T>(tileWidth , tileHeight),concurrentTiles, htgs::MMType::Dynamic);

            htgs::TaskGraphSignalHandler::registerTaskGraph(graph);
            htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

            auto *runtime = new htgs::TaskGraphRuntime(graph);
            runtime->executeRuntime();

            fi->requestAllTiles(true,pyramidLevelToRequestforThreshold);
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

//
//            htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs> *analyseGraph;  //< Analyse graph
//            htgs::TaskGraphRuntime *analyseRuntime; //< Analyse runtime
//
//
//            uint32_t pyramidLevelToRequestForSegmentation = 0;
//
//            //TODO for now we bypass the threshold selection mechanism and work directly with masks
//            auto tileLoader2 = new egt::PyramidTiledTiffLoader<T>(path, 2);
//            fi = new fi::FastImage<T>(tileLoader2, 2);
//            fi->getFastImageOptions()->setNumberOfViewParallel(10);
//            fi->configureAndRun();
//            imageHeight = fi->getImageHeight(pyramidLevelToRequestForSegmentation);
//            imageWidth = fi->getImageWidth(pyramidLevelToRequestForSegmentation);
//            tileWidth = fi->getTileWidth();
//
//            analyseGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs>;
//
//
//            auto sobelFilter2 = new FCSobelFilterOpenCV<T>(10, depth);
//            auto viewAnalyseTask = new egt::ViewAnalyser<T>(10,fi,4,threshold);
//            auto fileCreation = new BlobMerger(imageHeight,imageWidth,
//                    fi->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
//                    fi->getNumberTilesWidth(pyramidLevelToRequestForSegmentation));
//
//            analyseGraph->setGraphConsumerTask(sobelFilter2);
//            analyseGraph->addEdge(sobelFilter2,viewAnalyseTask);
//            analyseGraph->addEdge(viewAnalyseTask, fileCreation);
//            analyseGraph->addGraphProducerTask(fileCreation);
//            analyseRuntime = new htgs::TaskGraphRuntime(analyseGraph);
//            analyseRuntime->executeRuntime();
//
//            try {
//                //Request all tiles from the FC
//
//                VLOG(1) << "request all tiles...";
//                fi->requestAllTiles(true, pyramidLevelToRequestForSegmentation);
//                while (fi->isGraphProcessingTiles()) {
//                    auto pView = fi->getAvailableViewBlocking();
//                    if (pView != nullptr) {
//                        //Feed the graph with the views
//                        analyseGraph->produceData(pView);
//                    }
//                }
//            } catch (const fi::FastImageException &e) {
//                std::cerr << e.what() << std::endl;
//                exit(EXIT_FAILURE);
//            } catch (const std::exception &e) {
//                std::cerr << e.what() << std::endl;
//                exit(EXIT_FAILURE);
//            }
//
//            analyseGraph->finishedProducingData();
//
//            auto blob = analyseGraph->consumeData();
//
//            auto listblob = blob.get();
//
//            uint32_t nbBlobsTooSmall = 0;
//
//            auto originalNbOfBlobs = blob->_blobs.size();
//
//
//            auto i = listblob->_blobs.begin();
//            while (i != listblob->_blobs.end()) {
//                if((*i)->isForeground() && (*i)->getCount() < this->_minObjectSize){
////                    VLOG(1) << "too small...";
//                    nbBlobsTooSmall++;
//                    i = listblob->_blobs.erase(i);
//                }
//                else {
//                    i++;
//                }
//            }
//
//            auto nbBlobs = listblob->_blobs.size();
//            VLOG(1) << "original nb of blobs : " << originalNbOfBlobs;
//            VLOG(1) << "objects too small have been removed : " << nbBlobsTooSmall;
//            VLOG(1) << "blobs left : " <<nbBlobs;
//
//
//            FeatureCollection* fc = new FeatureCollection();
//            fc->createFCFromListBlobs(listblob, imageHeight, imageWidth);
//            fc->createBlackWhiteMask("output.tiff", tileWidth);
//
//
//            // Wait for the analyse graph to finish processing tiles to make the FC
//            // available
//            analyseRuntime->waitForRuntime();
//            fi->waitForGraphComplete();
//
//            analyseGraph->writeDotToFile("FeatureCollectionGraph.xdot", DOTGEN_COLOR_COMP_TIME);
//
//            // Delete HTGS graphs
//            delete (fi);
//            delete (analyseRuntime);

            auto end = std::chrono::high_resolution_clock::now();
            VLOG(1) << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " mS" << std::endl;



        }

    };

}


#endif //NEWEGT_EGT_H
