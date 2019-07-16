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


                //The first graph finds the threshold value used to segment the image
                ImageDepth depth = ImageDepth::_32F;
                uint32_t pyramidLevelToRequestforThreshold = 0;
                uint32_t concurrentTiles = 1;
                size_t nbLoaderThreads = 1;

                auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, nbLoaderThreads);
                auto *fi = new fi::FastImage<T>(tileLoader, 1);
                fi->getFastImageOptions()->setNumberOfViewParallel(concurrentTiles);
                auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");


                uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforThreshold);     //< Image Height
                uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforThreshold);      //< Image Width

                uint32_t tileWidth = fi->getTileWidth();
                uint32_t tileHeight = fi->getTileHeight();
                assert(tileWidth == tileHeight); //we work with square tiles

                auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
                auto sobelFilter = new CustomSobelFilter3by3<T>(concurrentTiles, depth);
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
                //TODO CHECK fi deleted by the graph when a TGTask?

                //the second graph segment the image
                htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs> *segmentationGraph;  //< Analyse graph
                htgs::TaskGraphRuntime *segmentationRuntime; //< Analyse runtime

                uint32_t pyramidLevelToRequestForSegmentation = 0;
                uint32_t segmentationRadius = 2; //radius of 2 since we need to check the ghost region for potential merges.

                auto tileLoader2 = new egt::PyramidTiledTiffLoader<T>(path, nbLoaderThreads);
                auto *fi2 = new fi::FastImage<T>(tileLoader2, segmentationRadius);
                fi2->getFastImageOptions()->setNumberOfViewParallel(concurrentTiles);
                auto fastImage2 = fi2->configureAndMoveToTaskGraphTask("Fast Image 2");
                uint32_t imageHeightAtSegmentationLevel = fi2->getImageHeight(pyramidLevelToRequestForSegmentation);     //< Image Height
                uint32_t imageWidthAtSegmentationLevel = fi2->getImageWidth(pyramidLevelToRequestForSegmentation);      //< Image Width
                int32_t tileHeigthAtSegmentationLevel = fi2->getTileHeight(pyramidLevelToRequestForSegmentation);
                int32_t tileWidthAtSegmentationLevel = fi2->getTileWidth(pyramidLevelToRequestForSegmentation);
                uint32_t nbTiles = fi2->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                                fi2->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);

                auto sobelFilter2 = new FCSobelFilterOpenCV<T>(concurrentTiles, depth);
                auto viewSegmentation = new egt::ViewAnalyser<T>(concurrentTiles,
                    imageHeightAtSegmentationLevel,
                    imageWidthAtSegmentationLevel,
                    tileHeigthAtSegmentationLevel,
                    tileWidthAtSegmentationLevel,
                    4,
                    threshold);
                auto merge = new BlobMerger(imageHeightAtSegmentationLevel,
                                               imageWidthAtSegmentationLevel,
                                               nbTiles);

                segmentationGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs>;
                segmentationGraph->addEdge(fastImage2,sobelFilter2);
                segmentationGraph->addEdge(sobelFilter2,viewSegmentation);
                segmentationGraph->addEdge(viewSegmentation, merge);
                segmentationGraph->addGraphProducerTask(merge);

                segmentationRuntime = new htgs::TaskGraphRuntime(segmentationGraph);
                segmentationRuntime->executeRuntime();

                //Request all tiles from the FC
                fi2->requestAllTiles(true, pyramidLevelToRequestForSegmentation);
                segmentationGraph->finishedProducingData();

                //we only generate one output
                auto blob = segmentationGraph->consumeData();

                if (blob != nullptr) {
                        auto listblob = blob->_blobs;

                        uint32_t nbBlobsTooSmall = 0;

                        auto originalNbOfBlobs = blob->_blobs.size();


                        auto i = blob->_blobs.begin();
                        while (i != blob->_blobs.end()) {
                                if((*i)->isForeground() && (*i)->getCount() < this->_minObjectSize){
        //                    VLOG(1) << "too small...";
                                        nbBlobsTooSmall++;
                                        i = blob->_blobs.erase(i);
                                }
                                else {
                                        i++;
                                }
                        }

                        auto nbBlobs = blob->_blobs.size();
                        VLOG(1) << "original nb of blobs : " << originalNbOfBlobs;
                        VLOG(1) << "objects too small have been removed : " << nbBlobsTooSmall;
                        VLOG(1) << "blobs left : " <<nbBlobs;
                }



                // Wait for the analyse graph to finish processing tiles to make the FC
                // available
                segmentationRuntime->waitForRuntime();


                segmentationGraph->writeDotToFile("FeatureCollectionGraph.xdot", DOTGEN_COLOR_COMP_TIME);
                // Delete HTGS graphs
                delete (segmentationRuntime);

                auto fc = new FeatureCollection();
                fc->createFCFromListBlobs(blob.get(), imageHeight, imageWidth);
                fc->createBlackWhiteMask("output.tiff", tileWidth);



                delete fc;


                auto end = std::chrono::high_resolution_clock::now();
                VLOG(1) << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " mS" << std::endl;



        }

    };

}


#endif //NEWEGT_EGT_H
