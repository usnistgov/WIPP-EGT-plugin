//
// Created by gerardin on 7/9/19.
//

#ifndef NEWEGT_EGT_H
#define NEWEGT_EGT_H


#include <string>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/FeatureCollection/Tasks/EGTViewAnalyzer.h>
#include <egt/FeatureCollection/Tasks/BlobMerger.h>
#include <egt/FeatureCollection/Tasks/FeatureCollection.h>
#include <egt/FeatureCollection/Tasks/ViewAnalyseFilter.h>
#include "DataTypes.h"
#include <egt/tasks/ThresholdFinder.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/FCSobelFilterOpenCV.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/tasks/FCCustomSobelFilter3by3.h>
#include <egt/memory/TileAllocator.h>
#include <egt/FeatureCollection/Tasks/ViewFilter.h>
#include <egt/tasks/TiffTileWriter.h>


namespace egt {


    class EGT {

    public:
        class EGTOptions {

        public:
            std::string inputPath{};
            std::string outputPath{};

            ImageDepth imageDepth = ImageDepth::_8U;

            size_t nbLoaderThreads = 1;
            uint32_t concurrentTiles = 1;

        };


    public:
        /// ----------------------------------
        /// The first graph finds the threshold value used to segment the image
        /// ----------------------------------
        template<class T>
        T runThresholdFinder(EGTOptions *options) {

            const uint32_t pyramidLevelToRequestforThreshold = 0;
            const uint32_t radiusForThreshold = 1;

            T threshold = 0;

            auto tileLoader = new egt::PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");


            uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforThreshold);     //< Image Height
            uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforThreshold);      //< Image Width
            uint32_t tileWidth = fi->getTileWidth();
            uint32_t tileHeight = fi->getTileHeight();
            uint32_t numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);
            uint32_t numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold);
            assert(tileWidth == tileHeight); //we work with square tiles

            auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
            auto sobelFilter = new CustomSobelFilter3by3<T>(options->concurrentTiles, options->imageDepth, 1, 1);
            auto thresholdFinder = new egt::ThresholdFinder<T>(imageWidth, imageHeight , numTileRow, numTileCol, options->imageDepth);
            graph->addEdge(fastImage,sobelFilter);
            graph->addEdge(sobelFilter,thresholdFinder);
            graph->addGraphProducerTask(thresholdFinder);

            //MEMORY MANAGEMENT
            graph->addMemoryManagerEdge("gradientTile",sobelFilter, new TileAllocator<T>(tileWidth , tileHeight),options->concurrentTiles, htgs::MMType::Dynamic);

//FOR DEBUGGING
                htgs::TaskGraphSignalHandler::registerTaskGraph(graph);
                htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

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
            graph->writeDotToFile("thresholdGraph.xdot", DOTGEN_COLOR_COMP_TIME);

            delete runtime; //this will also delete fastImage and the TileLoader

            return threshold;
        }

        template <class T>
        void runLocalMaskGenerator(T threshold, EGTOptions *options, SegmentationOptions *segmentationOptions){
            htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, VoidData> *localMaskGenerationGraph;
            htgs::TaskGraphRuntime *localMaskGenerationGraphRuntime;

            uint32_t pyramidLevelToRequestForSegmentation = 0;
            //radius of 2 since we need first apply convo, obtain a gradient of size n+1,
            //and then check the ghost region for potential merges for each tile of size n.
            uint32_t segmentationRadius = 2;

            auto tileLoader2 = new egt::PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi2 = new fi::FastImage<T>(tileLoader2, segmentationRadius);
            fi2->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage2 = fi2->configureAndMoveToTaskGraphTask("Fast Image 2");
            uint32_t imageHeightAtSegmentationLevel = fi2->getImageHeight(pyramidLevelToRequestForSegmentation);
            uint32_t imageWidthAtSegmentationLevel = fi2->getImageWidth(pyramidLevelToRequestForSegmentation);
            int32_t tileHeigthAtSegmentationLevel = fi2->getTileHeight(pyramidLevelToRequestForSegmentation);
            int32_t tileWidthAtSegmentationLevel = fi2->getTileWidth(pyramidLevelToRequestForSegmentation);
            uint32_t nbTiles = fi2->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                               fi2->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);

            auto sobelFilter2 = new FCCustomSobelFilter3by3<T>(options->concurrentTiles, options->imageDepth, 1, 1);
            auto viewSegmentation = new egt::EGTViewAnalyzer<T>(options->concurrentTiles,
                                                                imageHeightAtSegmentationLevel,
                                                                imageWidthAtSegmentationLevel,
                                                                tileHeigthAtSegmentationLevel,
                                                                tileWidthAtSegmentationLevel,
                                                                4,
                                                                threshold,
                                                                segmentationOptions);
            auto maskFilter = new ViewFilter<T>(options->concurrentTiles);
            auto merge = new BlobMerger(imageHeightAtSegmentationLevel,
                                        imageWidthAtSegmentationLevel,
                                        nbTiles);
            auto writeMask = new TiffTileWriter<T>(
                    1,
                    imageHeightAtSegmentationLevel,
                    imageWidthAtSegmentationLevel,
                    (uint32_t) tileHeigthAtSegmentationLevel,
                    ImageDepth::_8U,
                    outputPath + "mask.tif"
            );

            localMaskGenerationGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, VoidData>;
            localMaskGenerationGraph->addEdge(fastImage2, sobelFilter2);
            localMaskGenerationGraph->addEdge(sobelFilter2, viewSegmentation);
            localMaskGenerationGraph->addEdge(viewSegmentation, maskFilter);
            localMaskGenerationGraph->addEdge(maskFilter, writeMask);
            localMaskGenerationGraphRuntime = new htgs::TaskGraphRuntime(localMaskGenerationGraph);
            localMaskGenerationGraphRuntime->executeRuntime();
            fi2->requestAllTiles(true, pyramidLevelToRequestForSegmentation);
            localMaskGenerationGraph->finishedProducingData();
            localMaskGenerationGraphRuntime->waitForRuntime();
            //FOR DEBUGGING
            localMaskGenerationGraph->writeDotToFile("SegmentationGraph.xdot", DOTGEN_COLOR_COMP_TIME);
            delete (localMaskGenerationGraphRuntime);
            auto endSegmentation = std::chrono::high_resolution_clock::now();
        }


        template <class T>
        void runSegmentation(T threshold, EGTOptions *options, SegmentationOptions *segmentationOptions){
            htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs> *segmentationGraph;
            htgs::TaskGraphRuntime *segmentationRuntime;

            uint32_t pyramidLevelToRequestForSegmentation = 0;
            //radius of 2 since we need first apply convo, obtain a gradient of size n+1,
            //and then check the ghost region for potential merges for each tile of size n.
            uint32_t segmentationRadius = 2;

            auto tileLoader2 = new egt::PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi2 = new fi::FastImage<T>(tileLoader2, segmentationRadius);
            fi2->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage2 = fi2->configureAndMoveToTaskGraphTask("Fast Image 2");
            uint32_t imageHeightAtSegmentationLevel = fi2->getImageHeight(pyramidLevelToRequestForSegmentation);
            uint32_t imageWidthAtSegmentationLevel = fi2->getImageWidth(pyramidLevelToRequestForSegmentation);
            int32_t tileHeigthAtSegmentationLevel = fi2->getTileHeight(pyramidLevelToRequestForSegmentation);
            int32_t tileWidthAtSegmentationLevel = fi2->getTileWidth(pyramidLevelToRequestForSegmentation);
            uint32_t nbTiles = fi2->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                               fi2->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);

//                auto sobelFilter2 = new FCSobelFilterOpenCV<T>(concurrentTiles, imageDepth);
            auto sobelFilter2 = new FCCustomSobelFilter3by3<T>(options->concurrentTiles,options->imageDepth,1,1);

            auto viewSegmentation = new egt::EGTViewAnalyzer<T>(options->concurrentTiles,
                                                                imageHeightAtSegmentationLevel,
                                                                imageWidthAtSegmentationLevel,
                                                                tileHeigthAtSegmentationLevel,
                                                                tileWidthAtSegmentationLevel,
                                                                4,
                                                                threshold,
                                                                segmentationOptions);
            auto labelingFilter = new ViewAnalyseFilter<T>(options->concurrentTiles);
            auto merge = new BlobMerger(imageHeightAtSegmentationLevel,
                                        imageWidthAtSegmentationLevel,
                                        nbTiles);
            segmentationGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, ListBlobs>;
            segmentationGraph->addEdge(fastImage2,sobelFilter2);
            segmentationGraph->addEdge(sobelFilter2,viewSegmentation);
            segmentationGraph->addEdge(viewSegmentation, labelingFilter);
            segmentationGraph->addEdge(labelingFilter, merge);
            segmentationGraph->addGraphProducerTask(merge);

            htgs::TaskGraphSignalHandler::registerTaskGraph(segmentationGraph);
            htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

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
                            if((*i)->isForeground() && (*i)->getCount() < segmentationOptions->MIN_OBJECT_SIZE){
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
            segmentationGraph->writeDotToFile("SegmentationGraph.xdot", DOTGEN_COLOR_COMP_TIME);

            delete (segmentationRuntime);

            VLOG(1) << "generating a segmentation mask";
            beginFC = std::chrono::high_resolution_clock::now();
            auto fc = new FeatureCollection();
            fc->createFCFromListBlobs(blob.get(), imageHeightAtSegmentationLevel, imageWidthAtSegmentationLevel);
            fc->createBlackWhiteMask("output.tiff", (uint32_t)tileWidthAtSegmentationLevel);
            delete fc;
            endFC = std::chrono::high_resolution_clock::now();
        }


        /// This algorithm is divided in several graph, each fed by a different fast image.
        /// The reason is that we request different radius each time and each fastImage instance is associated
        /// with a single radius. It would be possible to rewrite the algo by taking the widest radius and calculate
        /// smaller radius from there, but marginal gains would not worth the complexity introduced.
        template<class T>
        void run(EGTOptions *options, SegmentationOptions *segmentationOptions) {

                auto begin = std::chrono::high_resolution_clock::now();

                auto beginThreshold = std::chrono::high_resolution_clock::now();
                T threshold = runThresholdFinder<T>(options);
//                T threshold = 88; //the threshold value to determine
                auto endThreshold = std::chrono::high_resolution_clock::now();


                auto beginSegmentation = std::chrono::high_resolution_clock::now();
                if(segmentationOptions->MASK_ONLY) {
                    runLocalMaskGenerator<T>(threshold, options, segmentationOptions);
                }
                else {
                    runSegmentation(threshold, options, segmentationOptions);
                }
                auto endSegmentation = std::chrono::high_resolution_clock::now();



                delete segmentationOptions;

                auto end = std::chrono::high_resolution_clock::now();


                VLOG(1) << "Execution time: ";
                VLOG(1) << "    Threshold Detection: " << std::chrono::duration_cast<std::chrono::milliseconds>(endThreshold - beginThreshold).count() << " mS";
                VLOG(1) << "    Segmentation: " << std::chrono::duration_cast<std::chrono::milliseconds>(endSegmentation - beginSegmentation).count() << " mS";
                VLOG(1) << "    Feature Collection: " << std::chrono::duration_cast<std::chrono::milliseconds>(endFC - beginFC).count() << " mS";
                VLOG(1) << "    Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " mS" << std::endl;



        }

        std::chrono::system_clock::time_point beginFC;
        std::chrono::system_clock::time_point endFC;

    };

}


#endif //NEWEGT_EGT_H
