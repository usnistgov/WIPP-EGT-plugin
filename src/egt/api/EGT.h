//
// Created by gerardin on 7/9/19.
//

#ifndef NEWEGT_EGT_H
#define NEWEGT_EGT_H


#include <string>
#include <egt/loaders/PyramidTiledTiffLoader.h>
#include <FastImage/api/FastImage.h>
#include <egt/FeatureCollection/Tasks/BlobMerger.h>
#include <egt/FeatureCollection/Tasks/FeatureCollection.h>
#include <egt/FeatureCollection/Tasks/ViewAnalyseFilter.h>
#include "DataTypes.h"
#include <egt/tasks/FastThresholdFinder.h>
#include <egt/tasks/ThresholdFinder.h>
#include <egt/tasks/SobelFilterOpenCV.h>
#include <egt/tasks/FCSobelFilterOpenCV.h>
#include <htgs/log/TaskGraphSignalHandler.hpp>
#include <egt/tasks/CustomSobelFilter3by3.h>
#include <egt/tasks/FCCustomSobelFilter3by3.h>
#include <egt/memory/TileAllocator.h>
#include <egt/FeatureCollection/Tasks/ViewFilter.h>
#include <egt/tasks/TiffTileWriter.h>
#include <egt/api/EGTOptions.h>
#include <random>
#include <egt/tasks/EGTSobelFilter.h>
#include <egt/tasks/NoTransform.h>
#include <egt/FeatureCollection/Tasks/EGTGradientViewAnalyzer.h>
#include "DerivedSegmentationParams.h"
#include <experimental/filesystem>
#include <egt/utils/PixelIntensityBoundsFinder.h>
#include <egt/utils/pyramid/Pyramid.h>
#include <egt/utils/pyramid/RecursiveBlockTraversal.h>
#include <egt/rules/MergeRule.h>
#include <egt/FeatureCollection/Tasks/MergeBlob.h>
#include <egt/rules/MergeCompletedRule.h>
#include <egt/FeatureCollection/Tasks/FeatureBuilder.h>


namespace egt {

    namespace fs = std::experimental::filesystem;

    class EGTInput : public htgs::IData {

    public:
        EGTInput(egt::EGTOptions *options, egt::SegmentationOptions *segmentationOptions,
                 std::map <std::string, uint32_t> &expertModeOptions) : options(options),
                                                                        segmentationOptions(segmentationOptions),
                                                                        expertModeOptions(expertModeOptions) {}

    public:
        EGTOptions *options;
        SegmentationOptions *segmentationOptions;
        std::map <std::string, uint32_t> &expertModeOptions;
    };


    class EGTOutput : public IData {
    };


    template<class T>
    class EGT  : public ITask<EGTInput,EGTOutput> {

    public:

    private:
        void executeTask(std::shared_ptr<EGTInput> data) override {
            run(data->options, data->segmentationOptions, data->expertModeOptions);
            this->addResult(new EGTOutput);
        }

        ITask<EGTInput, EGTOutput> *copy() override {
            return this;
        }

    public:

/// This algorithm is divided in several graph, each fed by a different fast image.
        /// The reason is that we request different radius each time and each fastImage instance is associated
        /// with a single radius. It would be possible to rewrite the algo by taking the widest radius and calculate
        /// smaller radius from there, but marginal gains would not worth the complexity introduced.
        void run(EGTOptions *options, SegmentationOptions *segmentationOptions,
                 std::map<std::string, uint32_t> &expertModeOptions) {

            auto begin = std::chrono::high_resolution_clock::now();

            //Reading from configuration extra parameters for the algorithm execution.
            options->nbLoaderThreads = (expertModeOptions.find("loader") != expertModeOptions.end())
                                       ? expertModeOptions.at("loader") : 1;
            options->concurrentTiles = (expertModeOptions.find("tile") != expertModeOptions.end())
                                       ? expertModeOptions.at("tile") : 1;
            options->nbTilePerSample = (expertModeOptions.find("sample") != expertModeOptions.end()) ? expertModeOptions.at(
                    "sample") : -1;
            options->nbExperiments = (expertModeOptions.find("exp") != expertModeOptions.end()) ? expertModeOptions.at(
                    "exp") : -1;
            options->threshold = (expertModeOptions.find("threshold") != expertModeOptions.end())
                                 ? expertModeOptions.at("threshold") : -1;
            options->pixelIntensityBoundsLevelUp = (expertModeOptions.find("intensitylevel") != expertModeOptions.end())
                    ? expertModeOptions.at("intensitylevel") : 0;

            options->rank = (expertModeOptions.find("rank") != expertModeOptions.end())
                                 ? expertModeOptions.at("rank") : 4;

            options->streamingWrite = (expertModeOptions.find("streaming") != expertModeOptions.end())
                            ? expertModeOptions.at("streaming") == 1 : false;

            options->erode = (expertModeOptions.find("erode") != expertModeOptions.end())
                                      ? expertModeOptions.at("erode") == 1 : true;

            VLOG(1) << "Execution model : ";
            VLOG(1) << "loader threads : " << options->nbLoaderThreads;
            VLOG(1) << "concurrent tiles : " << options->concurrentTiles;
            VLOG(1) << "fixed threshold : " << std::boolalpha << (options->threshold != -1);
            if (options->threshold != -1) {
                VLOG(1) << "fixed threshold value : " << options->threshold << std::endl;
            }
            VLOG(1) << "threshold is calculated at pyramid level: " << options->pyramidLevel;
            if (options->nbTilePerSample != -1) {
                VLOG(1) << "Threshold finder. Nb of tiles per sample requested: " << options->nbTilePerSample << std::endl;
            }
            if (options->nbExperiments != -1) {
                VLOG(1) << "Threshold finder. Nb of experiments requested: " << options->nbExperiments << std::endl;
            }
            VLOG(1) << "min and max intensity are calculated at pyramid level: " << options->pixelIntensityBoundsLevelUp;
            VLOG(1) << "performing erosion: " << std::boolalpha << options->erode;


            //We need to derive the segmentations params from the user defined parameters
            auto beginIntensityFilter = std::chrono::high_resolution_clock::now();
            //Finding intensity bounds.
            auto segmentationParams = DerivedSegmentationParams<T>();
            if(! segmentationOptions->disableIntensityFilter) {
               auto intensityFinder = new PixelIntensityBoundsFinder<T>();
               intensityFinder->runFastPixelIntensityBounds(options, segmentationOptions, segmentationParams);
               delete intensityFinder;
            }
            auto endIntensityFilter = std::chrono::high_resolution_clock::now();

            //Finding gradient threshold pixel value.
            auto beginThreshold = std::chrono::high_resolution_clock::now();
            T threshold{};
            if (options->threshold == -1) {
                threshold = runThresholdFinder(options);
            } else {
                threshold = options->threshold;
            }
            auto endThreshold = std::chrono::high_resolution_clock::now();
            segmentationParams.threshold = threshold;

            //Segmentation
            std::list<std::shared_ptr<Feature>> features{};
            auto beginSegmentation = std::chrono::high_resolution_clock::now();
            if (segmentationOptions->MASK_ONLY) {
                runLocalMaskGenerator(threshold, options, segmentationOptions, segmentationParams);
            } else {
                features = runSegmentation(threshold, options, segmentationOptions, segmentationParams);
            }
            auto endSegmentation = std::chrono::high_resolution_clock::now();

            //Mask generation
            auto beginFC = std::chrono::high_resolution_clock::now();
            auto fc = new FeatureCollection();
            fc->createFCFromFeatures(features, imageHeightAtSegmentationLevel, imageWidthAtSegmentationLevel);

            fs::path path = options->inputPath;
            std::string inputFilename = path.filename();
            std::string outputFilenamePrefix = "";
            outputFilenamePrefix = "bw-mask-";
            auto outputFilename = outputFilenamePrefix + inputFilename;
            auto outputFilepath =  (fs::path(options->outputPath) / outputFilename).string();

            fc->createBlackWhiteMask(outputFilepath, (uint32_t) tileWidthAtSegmentationLevel);
//            if (!segmentationOptions->MASK_ONLY) {
//                if(options->erode) {
//                    blobs->erode(options, segmentationOptions);
//                }
//                runMaskGeneration(blobs, options, segmentationOptions);
//            }

            VLOG(4) << "image written to : " << outputFilepath;

            auto endFC = std::chrono::high_resolution_clock::now();

            auto end = std::chrono::high_resolution_clock::now();

            //print out summary
            VLOG(1) << "Execution time: ";
            VLOG(1) << "    Intensity Filter Bounds Detection: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(endIntensityFilter - beginIntensityFilter).count()
                    << " mS";
            VLOG(1) << "    Threshold Detection: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(endThreshold - beginThreshold).count()
                    << " mS";
            VLOG(1) << "    Segmentation: " << std::chrono::duration_cast<std::chrono::milliseconds>(
                    endSegmentation - beginSegmentation).count() << " mS";
            VLOG(1) << "    Feature Collection: "
                    << std::chrono::duration_cast<std::chrono::milliseconds>(endFC - beginFC).count() << " mS";
            VLOG(1) << "    Total: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
                    << " mS" << std::endl;
        }



        /// ----------------------------------
        /// The first graph finds the threshold value used to segment the image
        /// ----------------------------------
        T runThresholdFinder(EGTOptions *options) {

            const uint32_t pyramidLevelToRequestforThreshold = options->pyramidLevel;
            const uint32_t radiusForThreshold = 1;

            T threshold = 0;

            auto tileLoader = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader, radiusForThreshold);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage = fi->configureAndMoveToTaskGraphTask("Fast Image");


            uint32_t imageHeight = fi->getImageHeight(pyramidLevelToRequestforThreshold);     //< Image Height
            uint32_t imageWidth = fi->getImageWidth(pyramidLevelToRequestforThreshold);      //< Image Width
            uint32_t tileWidth = fi->getTileWidth();
            uint32_t tileHeight = fi->getTileHeight();
            assert(tileWidth == tileHeight); //we work with square tiles
            uint32_t numTileCol = fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);
            uint32_t numTileRow = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold);
            uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestforThreshold) *
                               fi->getNumberTilesWidth(pyramidLevelToRequestforThreshold);

            auto randomExperiments = true;
            if(options->nbTilePerSample == -1 && options->nbExperiments == -1) {
                randomExperiments = false;
            }
            auto nbOfSamples = (options->nbTilePerSample != -1) ? std::min((int32_t)nbTiles, options->nbTilePerSample) : nbTiles;
            auto nbOfSamplingExperiment = (size_t)((options->nbExperiments != -1) ? options->nbExperiments : 1);

            VLOG(1) << "Threshold finder. Nb of tiles per sample: " << nbOfSamples << std::endl;
            VLOG(1) << "Threshold finder. Nb of experiments: " << nbOfSamplingExperiment << std::endl;

            auto graph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Threshold<T>>();
            auto sobelFilter = new CustomSobelFilter3by3<T>(options->concurrentTiles, options->imageDepth, 1, 1);

            auto thresholdFinder = new FastThresholdFinder<T>(nbOfSamplingExperiment, tileHeight * tileWidth, nbOfSamples,
                                                          options->imageDepth);
            graph->addEdge(fastImage, sobelFilter);
            graph->addEdge(sobelFilter, thresholdFinder);
            graph->addGraphProducerTask(thresholdFinder);

            //MEMORY MANAGEMENT
            graph->addMemoryManagerEdge("gradientTile", sobelFilter, new TileAllocator<T>(tileWidth, tileHeight),
                                        options->concurrentTiles, htgs::MMType::Dynamic);

            //FOR DEBUGGING
            htgs::TaskGraphSignalHandler::registerTaskGraph(graph);
            htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

            auto *runtime = new htgs::TaskGraphRuntime(graph);
            runtime->executeRuntime();

            if(randomExperiments) {
                auto seed1 = std::chrono::system_clock::now().time_since_epoch().count();
                std::default_random_engine generator(seed1);
                std::uniform_int_distribution<uint32_t> distributionRowRange(0, numTileRow - 1);
                std::uniform_int_distribution<uint32_t> distributionColRange(0, numTileCol - 1);
                for (auto i = 0; i < nbOfSamplingExperiment * nbOfSamples; i++) {
                    uint32_t randomRow = distributionRowRange(generator);
                    uint32_t randomCol = distributionColRange(generator);
                    VLOG(3) << "Requesting tile : (" << randomRow << ", " << randomCol << ")";
                    fi->requestTile(randomRow, randomCol, false, pyramidLevelToRequestforThreshold);
                }
            }
            else {
                fi->requestAllTiles(false,pyramidLevelToRequestforThreshold);
            }

            fi->finishedRequestingTiles();
            graph->finishedProducingData();

            std::vector<T> thresholdCandidates = {};

            while (!graph->isOutputTerminated()) {
                auto data = graph->consumeData();
                if (data != nullptr) {
                    thresholdCandidates.push_back(data->getValue());
                }
            }

            double sum = 0;
            for (T t : thresholdCandidates) {
                sum += t;
            }
            threshold = sum / thresholdCandidates.size();
            VLOG(3) << "Threshold value using average: " << (double)threshold;

            std::sort(thresholdCandidates.begin(), thresholdCandidates.end());
            auto medianIndex = std::ceil(thresholdCandidates.size() / 2);
            threshold = thresholdCandidates[medianIndex];
            VLOG(3) << "Threshold value using median : " << (double)threshold;

            VLOG(1) << "Threshold value : " << (double)threshold;

            runtime->waitForRuntime();
//FOR DEBUGGING
            graph->writeDotToFile("thresholdGraph.xdot", DOTGEN_COLOR_COMP_TIME);

            delete fi;
            delete runtime;

            return threshold;
        }

        /**
         * Generate a binary mask encoded in uint8 from a threshold.
         * The thresholding is performed for each image tile without global processing.
         * This is very fast but cannot be 100% correct as features spanning several tiles will not be filtered.
         * @tparam T
         * @param threshold - Threshold value for the foreground.
         * @param options - Options for configuring EGT execution.
         * @param segmentationOptions - Parameters used for the segmentation.
         */
        void runLocalMaskGenerator(T threshold, EGTOptions *options, SegmentationOptions *segmentationOptions, DerivedSegmentationParams<T> segmentationParams) {
            htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, VoidData> *localMaskGenerationGraph;
            htgs::TaskGraphRuntime *localMaskGenerationGraphRuntime;

            uint32_t pyramidLevelToRequestForSegmentation = options->pyramidLevel;
            //radius of 2 since we need first apply convo, obtain a gradient of size n+1,
            //and then check the ghost region for potential merges for each tile of size n.
            uint32_t segmentationRadius = 2;

            auto tileLoader2 = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader2, segmentationRadius);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImageTask = fi->configureAndMoveToTaskGraphTask("Fast Image");
            uint32_t imageHeightAtSegmentationLevel = fi->getImageHeight(pyramidLevelToRequestForSegmentation);
            uint32_t imageWidthAtSegmentationLevel = fi->getImageWidth(pyramidLevelToRequestForSegmentation);
            int32_t tileHeigthAtSegmentationLevel = fi->getTileHeight(pyramidLevelToRequestForSegmentation);
            int32_t tileWidthAtSegmentationLevel = fi->getTileWidth(pyramidLevelToRequestForSegmentation);
            uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                               fi->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);

            auto sobelFilter2 = new EGTSobelFilter<T>(options->concurrentTiles, options->imageDepth, 1, 1);
            auto viewSegmentation = new EGTGradientViewAnalyzer<T>(options->concurrentTiles,
                                                           imageHeightAtSegmentationLevel,
                                                           imageWidthAtSegmentationLevel,
                                                           tileHeigthAtSegmentationLevel,
                                                           tileWidthAtSegmentationLevel,
                                                           options->rank,
                                                           threshold,
                                                           segmentationOptions,
                                                           segmentationParams);
            auto maskFilter = new ViewFilter<T>(options->concurrentTiles);
            auto merge = new BlobMerger<T>(imageHeightAtSegmentationLevel,
                                        imageWidthAtSegmentationLevel,
                                        nbTiles,
                                        options,
                                        segmentationOptions,
                                        segmentationParams);
            auto writeMask = new TiffTileWriter<T>(
                    1,
                    imageHeightAtSegmentationLevel,
                    imageWidthAtSegmentationLevel,
                    (uint32_t) tileHeigthAtSegmentationLevel,
                    ImageDepth::_8U,
                    (fs::path(options->outputPath) / "mask.tif").string()
            );

            localMaskGenerationGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, VoidData>;
            localMaskGenerationGraph->addEdge(fastImageTask, sobelFilter2);
            localMaskGenerationGraph->addEdge(sobelFilter2, viewSegmentation);
            localMaskGenerationGraph->addEdge(viewSegmentation, maskFilter);
            localMaskGenerationGraph->addEdge(maskFilter, writeMask);
            localMaskGenerationGraphRuntime = new htgs::TaskGraphRuntime(localMaskGenerationGraph);
            localMaskGenerationGraphRuntime->executeRuntime();
            fi->requestAllTiles(true, pyramidLevelToRequestForSegmentation);
            localMaskGenerationGraph->finishedProducingData();
            localMaskGenerationGraphRuntime->waitForRuntime();
            //FOR DEBUGGING
            localMaskGenerationGraph->writeDotToFile("SegmentationGraph.xdot", DOTGEN_COLOR_COMP_TIME);
            delete fi;
            delete localMaskGenerationGraphRuntime;
            auto endSegmentation = std::chrono::high_resolution_clock::now();
        }

        /**
         * Generate a list of features.
         * @tparam T
         * @param threshold - Threshold value for the foreground.
         * @param options - Options for configuring EGT execution.
         * @param segmentationOptions - Parameters used for the segmentation.
         */
        std::list<std::shared_ptr<Feature>>
        runSegmentation(T threshold, EGTOptions *options, SegmentationOptions *segmentationOptions, DerivedSegmentationParams<T> segmentationParams) {
            htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Feature> *segmentationGraph;
            htgs::TaskGraphRuntime *segmentationRuntime;

            uint32_t pyramidLevelToRequestForSegmentation = options->pyramidLevel;
            //radius of 2 since we need first apply convo, obtain a gradient of size n+1,
            //and then check the ghost region for potential merges for each tile of size n.
            uint32_t segmentationRadius = 2;


            auto tileLoader2 = new PyramidTiledTiffLoader<T>(options->inputPath, options->nbLoaderThreads);
            auto *fi = new fi::FastImage<T>(tileLoader2, segmentationRadius);
            fi->getFastImageOptions()->setNumberOfViewParallel(options->concurrentTiles);
            auto fastImage2 = fi->configureAndMoveToTaskGraphTask("Fast Image 2");
            imageHeightAtSegmentationLevel = fi->getImageHeight(pyramidLevelToRequestForSegmentation);
            imageWidthAtSegmentationLevel = fi->getImageWidth(pyramidLevelToRequestForSegmentation);
            tileHeightAtSegmentationLevel = fi->getTileHeight(pyramidLevelToRequestForSegmentation);
            tileWidthAtSegmentationLevel = fi->getTileWidth(pyramidLevelToRequestForSegmentation);
            uint32_t nbTiles = fi->getNumberTilesHeight(pyramidLevelToRequestForSegmentation) *
                               fi->getNumberTilesWidth(pyramidLevelToRequestForSegmentation);
            auto pyramid = pb::Pyramid(fi->getImageWidth(), fi->getImageHeight(), fi->getTileWidth());

            auto sobelFilter2 = new EGTSobelFilter<T>(options->concurrentTiles, options->imageDepth, 1, 1);
//            auto sobelFilter2 = new NoTransform<T>(options->concurrentTiles, options->imageDepth, 1, 1);

            auto viewSegmentation = new EGTGradientViewAnalyzer<T>(options->concurrentTiles,
                                                           imageHeightAtSegmentationLevel,
                                                           imageWidthAtSegmentationLevel,
                                                           tileHeightAtSegmentationLevel,
                                                           tileWidthAtSegmentationLevel,
                                                           options->rank,
                                                           threshold,
                                                           segmentationOptions,
                                                           segmentationParams);
            auto labelingFilter = new ViewAnalyseFilter<T>(options->concurrentTiles);

            auto bookkeeper = new htgs::Bookkeeper<ViewAnalyse>();

            auto mergeCompletedRule = new MergeCompletedRule(pyramid);
            auto mergeRule = new MergeRule(pyramid);


            auto featureBuilder = new FeatureBuilder(options->concurrentTiles);

            segmentationGraph = new htgs::TaskGraphConf<htgs::MemoryData<fi::View<T>>, Feature>;
            segmentationGraph->addEdge(fastImage2, sobelFilter2);
            segmentationGraph->addEdge(sobelFilter2, viewSegmentation);
            segmentationGraph->addEdge(viewSegmentation, labelingFilter);


            auto mergeBlob = new MergeBlob(options->concurrentTiles);


            segmentationGraph->addEdge(labelingFilter, bookkeeper);
            segmentationGraph->addRuleEdge(bookkeeper, mergeCompletedRule, featureBuilder);
            segmentationGraph->addRuleEdge(bookkeeper,mergeRule, mergeBlob);
            segmentationGraph->addEdge(mergeBlob,bookkeeper);


            segmentationGraph->addGraphProducerTask(featureBuilder);

            htgs::TaskGraphSignalHandler::registerTaskGraph(segmentationGraph);
            htgs::TaskGraphSignalHandler::registerSignal(SIGTERM);

            segmentationRuntime = new htgs::TaskGraphRuntime(segmentationGraph);
            segmentationRuntime->executeRuntime();
            fi->requestAllTiles(true, pyramidLevelToRequestForSegmentation);


            auto traversal = new pb::RecursiveBlockTraversal(pyramid);
            for(auto step : traversal->getTraversal()){
                auto row = step.first;
                auto col = step.second;
                fi->requestTile(row,col,false,0);
                VLOG(5) << "Requesting tile (" << row << "," << col << ")";
            }

            segmentationGraph->finishedProducingData();

            std::list<std::shared_ptr<Feature>> features{};

            while(! segmentationGraph->isOutputTerminated()) {
                auto feature = segmentationGraph->consumeData();
                if(feature != nullptr){
                    features.push_back(feature);
                }
            }

            VLOG(4) << " Total number of features produced: " << features.size();

            segmentationRuntime->waitForRuntime();
            delete traversal;
            delete fi;
            delete segmentationRuntime;
          return features;
        }

        void runMaskGeneration(std::shared_ptr<ListBlobs> &blob, EGTOptions *options,
                               SegmentationOptions *segmentationOptions) {
            VLOG(1) << "generating a segmentation mask";
            auto fc = new FeatureCollection();
            fc->createFCFromCompactListBlobs(blob.get(), imageHeightAtSegmentationLevel, imageWidthAtSegmentationLevel);

            //TODO CHECK WHAT TO DO. This is the fast way of writing a image but also memory hungry (needs to load all tiles in memory).
            //TODO should we make it an option?
       //     fc->createBlackWhiteMask("output.tiff", (uint32_t) tileWidthAtSegmentationLevel);

            fs::path path = options->inputPath;
            std::string inputFilename = path.filename();
            std::string outputFilenamePrefix = "";

            if(fs::create_directories(fs::path(options->outputPath))){
                VLOG(4) << "Directory  created: " << options->outputPath;
            }

            //generating a labeled mask. Let's try to find the appropriate resolution we need to correctly render each feature.
            if(options->label) {
                auto nbBlobs = blob->_blobs.size();
                auto depth = ImageDepth::_32U;

                outputFilenamePrefix = "labeled-mask-";
                auto outputFilename = outputFilenamePrefix + inputFilename;
                auto outputFilepath =  (fs::path(options->outputPath) / outputFilename).string();


                if(options->streamingWrite) {
                    if (nbBlobs < 256) {
                        depth = ImageDepth::_8U;
                        fc->createLabeledMaskStreaming<uint8_t>(outputFilepath,
                                                                (uint32_t) tileWidthAtSegmentationLevel,
                                                                depth);
                    } else if (nbBlobs < 256 * 256) {
                        depth = ImageDepth::_16U;
                        fc->createLabeledMaskStreaming<uint16_t>(outputFilepath,
                                                                 (uint32_t) tileWidthAtSegmentationLevel,
                                                                 depth);
                    } else {
                        fc->createLabeledMaskStreaming<uint32_t>(outputFilepath,
                                                                 (uint32_t) tileWidthAtSegmentationLevel,
                                                                 depth);
                    }
                }
                else {
                    fc->createLabeledMask(outputFilepath);
                }
            }
            else {
                outputFilenamePrefix = "bw-mask-";
                auto outputFilename = outputFilenamePrefix + inputFilename;
                auto outputFilepath =  (fs::path(options->outputPath) / outputFilename).string();

                if(options->streamingWrite) {
                    fc->createBlackWhiteMaskStreaming(outputFilepath, (uint32_t) tileWidthAtSegmentationLevel);
                }
                else{
                    fc->createBlackWhiteMask(outputFilepath, (uint32_t) tileWidthAtSegmentationLevel);
                }
            }

            delete fc;
        }


        void computeMeanIntensities(std::shared_ptr<ListBlobs> &blobs, EGTOptions *options) {

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

            for (auto blob : blobs->_blobs) {
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
                assert(sum != 0);
                featureMeanIntensity = std::round(sum / count);
                meanIntensities[blob] = featureMeanIntensity;
            }

            fi->finishedRequestingTiles();
            fi->waitForGraphComplete();
            delete fi;
        }


    private:
        uint32_t imageHeightAtSegmentationLevel{},
                imageWidthAtSegmentationLevel{},
                tileWidthAtSegmentationLevel{},
                tileHeightAtSegmentationLevel{};


        std::map<Blob*, T> meanIntensities{};


    };


}


#endif //NEWEGT_EGT_H
