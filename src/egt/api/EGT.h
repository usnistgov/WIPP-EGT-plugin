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


    class EGT {

    private:

        uint32_t MIN_OBJECT_SIZE = 3000;
        uint32_t MIN_HOLE_SIZE = 3000;


    public:


        /// This algorithm is divided in several graph, each fed by a different fast image.
        /// The reason is that we request different radius each time and each fastImage instance is associated
        /// with a single radius. It would be possible to rewrite the algo by taking the widest radius and calculate
        /// smaller radius from there, but it probably do not worth the complexity introduced.
        template<class T>
        void run(std::string path, ImageDepth depth) {



                auto begin = std::chrono::high_resolution_clock::now();

                size_t nbLoaderThreads = 2;
                uint32_t concurrentTiles = 10;

                //----------------------------------
                //The first graph finds the threshold value used to segment the image
                //----------------------------------
                uint32_t pyramidLevelToRequestforThreshold = 0;
                const uint32_t radiusForThreshold = 1;

                T threshold = 0; //the threshold value to determine

                auto tileLoader = new egt::PyramidTiledTiffLoader<T>(path, nbLoaderThreads);
                auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
                fi->getFastImageOptions()->setNumberOfViewParallel(concurrentTiles);
                auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");


                uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforThreshold);     //< Image Height
                uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforThreshold);      //< Image Width
                uint32_t tileWidth = fi->getTileWidth();
                uint32_t tileHeight = fi->getTileHeight();
                uint32_t numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);
                uint32_t numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold);
                assert(tileWidth == tileHeight); //we work with square tiles

                auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
                auto sobelFilter = new CustomSobelFilter3by3<T>(concurrentTiles, depth);
                auto thresholdFinder = new egt::ThresholdFinder<T>(imageWidth, imageHeight , numTileRow, numTileCol);
                graph->addEdge(fastImage,sobelFilter);
                graph->addEdge(sobelFilter,thresholdFinder);
                graph->addGraphProducerTask(thresholdFinder);

                //MEMORY MANAGEMENT
                graph->addMemoryManagerEdge("gradientTile",sobelFilter, new TileAllocator<T>(tileWidth , tileHeight),concurrentTiles, htgs::MMType::Dynamic);

//FOR DEBUGGING
//                htgs::TaskGraphSignalHandler::registerTaskGraph(graph);
//                htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

                auto *runtime = new htgs::TaskGraphRuntime(graph);
                runtime->executeRuntime();
                fi->requestAllTiles(true,pyramidLevelToRequestforThreshold);
                graph->finishedProducingData();

                while (!graph->isOutputTerminated()) {
                        auto data = graph->consumeData();
                        if (data != nullptr) {
                            threshold = data->getValue();
                            VLOG(1) << "Threshold value : " << threshold;
                        }
                }

                runtime->waitForRuntime();
//FOR DEBUGGING
//                graph->writeDotToFile("colorGraph.xdot", DOTGEN_COLOR_COMP_TIME);
                delete runtime; //this will also delete fastImage and the TileLoader


                //----------------------------------
                //the second graph segment the image
                //----------------------------------
                auto options = new SegmentationOptions();
                options->setMinHoleSize(MIN_HOLE_SIZE);
                options->setMinObjectSize(MIN_OBJECT_SIZE);

                VLOG(3) << options;


                htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs> *segmentationGraph;
                htgs::TaskGraphRuntime *segmentationRuntime;

                uint32_t pyramidLevelToRequestForSegmentation = 0;
                //radius of 2 since we need first apply convo, obtain a gradient of size n+1,
                //and then check the ghost region for potential merges for each tile of size n.
                uint32_t segmentationRadius = 2;

                auto tileLoader2 = new egt::PyramidTiledTiffLoader<T>(path, nbLoaderThreads);
                auto *fi2 = new fi::FastImage<T>(tileLoader2, segmentationRadius);
                fi2->getFastImageOptions()->setNumberOfViewParallel(concurrentTiles);
                auto fastImage2 = fi2->configureAndMoveToTaskGraphTask("Fast Image 2");
                uint32_t imageHeightAtSegmentationLevel = fi2->getImageHeight(pyramidLevelToRequestForSegmentation);
                uint32_t imageWidthAtSegmentationLevel = fi2->getImageWidth(pyramidLevelToRequestForSegmentation);
                uint32_t tileHeigthAtSegmentationLevel = fi2->getTileHeight(pyramidLevelToRequestForSegmentation);
                uint32_t tileWidthAtSegmentationLevel = fi2->getTileWidth(pyramidLevelToRequestForSegmentation);
                uint32_t nbTiles = fi2->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                                fi2->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);

                auto sobelFilter2 = new FCSobelFilterOpenCV<T>(concurrentTiles, depth);
                auto viewSegmentation = new egt::ViewAnalyser<T>(concurrentTiles,
                    imageHeightAtSegmentationLevel,
                    imageWidthAtSegmentationLevel,
                    static_cast<int32_t>(tileHeigthAtSegmentationLevel),
                    static_cast<int32_t>(tileWidthAtSegmentationLevel),
                    4,
                    threshold);
                viewSegmentation->setSegmentationOptions(options);
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
                fi2->requestAllTiles(true, pyramidLevelToRequestForSegmentation);
                segmentationGraph->finishedProducingData();

                //we only generate one output, the list of all objects
                auto blob = segmentationGraph->consumeData();
                if (blob != nullptr) {
                        auto listblob = blob->_blobs;

                        uint32_t nbBlobsTooSmall = 0;

                        auto originalNbOfBlobs = blob->_blobs.size();


                        auto i = blob->_blobs.begin();
                        while (i != blob->_blobs.end()) {
                                //We removed objects that are still too small after the merge occured.
                                if((*i)->isForeground() && (*i)->getCount() < this->MIN_OBJECT_SIZE){
                                        nbBlobsTooSmall++;
                                        i = blob->_blobs.erase(i);
                                }
                                else {
                                        i++;
                                }
                        }

                        auto nbBlobs = blob->_blobs.size();
                        VLOG(3) << "original nb of blobs : " << originalNbOfBlobs;
                        VLOG(3) << "nb of small objects that have been removed : " << nbBlobsTooSmall;
                        VLOG(1) << "total nb of objects: " <<nbBlobs;
                }
                // Wait for the analyse graph to finish processing tiles to make the FC
                // available
                segmentationRuntime->waitForRuntime();

//FOR DEBUGGING
//segmentationGraph->writeDotToFile("FeatureCollectionGraph.xdot", DOTGEN_COLOR_COMP_TIME);

                // Delete HTGS graphs
                delete (segmentationRuntime);
                delete options;

                VLOG(1) << "generating a segmentation mask";
                auto fc = new FeatureCollection();
                fc->createFCFromListBlobs(blob.get(), imageHeightAtSegmentationLevel, imageWidthAtSegmentationLevel);
                fc->createBlackWhiteMask("output.tiff", tileWidthAtSegmentationLevel);

                delete fc;

                auto end = std::chrono::high_resolution_clock::now();
                VLOG(1) << "Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " mS" << std::endl;



        }

    };

}


#endif //NEWEGT_EGT_H
