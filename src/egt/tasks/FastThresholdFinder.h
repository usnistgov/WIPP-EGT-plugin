//
// Created by gerardin on 9/5/19.
//

#ifndef NEWEGT_FASTTHRESHOLDFINDER_H
#define NEWEGT_FASTTHRESHOLDFINDER_H

#include <egt/data/Threshold.h>
#include <egt/data/ConvOutMemoryData.h>
#include <htgs/api/ITask.hpp>
#include <algorithm>
#include <glog/logging.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>
#include <opencv/cv.hpp>
#include <egt/utils/Utils.h>

namespace egt {


    std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
//    std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

    template <class T>
    class FastThresholdFinder : public htgs::ITask<ConvOutMemoryData<T>, Threshold<T>> {


    public:
        FastThresholdFinder(size_t numThreads, uint32_t sampleSize, uint32_t numberOfSamples, ImageDepth imageDepth) : htgs::ITask<ConvOutMemoryData<T>, Threshold<T>>(numThreads),
                                                                                                                   sampleSize(sampleSize),
                                                                                                                   nbOfSamples(numberOfSamples),
                                                                                                                   imageDepth(imageDepth) {
//        gradient = std::vector<T>(nbOfSamples * sampleSize, 0);
            //          gradient.reserve(nbOfSamples * sampleSize);
            hist= std::vector<double>(NUM_HISTOGRAM_BINS + 1, 0);

            hist.reserve(NUM_HISTOGRAM_BINS + 1);

            //TODO only works for 16bits images
            size_t size = 0;
            switch(imageDepth){
                case (ImageDepth::_8U) :
                    size = 256;
                    break;
                case (ImageDepth::_16U) :
                    size = 256 * 256;
                    break;
                default:
                    throw "Image Depth is too large for this approach.";
            }
            bins = std::vector<double>(size);
        }

        void executeTask(std::shared_ptr<ConvOutMemoryData<T>> data) override {

            binSample(data);
            data->getOutputdata()->releaseMemory();

            counter++;

            if(counter == nbOfSamples) {

                VLOG(3) << "Determining Threshold..." ;

                VLOG(4) << "we have run gradient on : " << nbOfSamples << " sample tiles.";

                //create an histogram of NUM_HISTOGRAM_BINS bins
                //we also collect none zero values as we need them for the final step when applying the percentileThreshold.

                VLOG(4) << "Nb of gradient pixels : " << nbOfSamples * sampleSize;
                VLOG(4) << "min : " << minValue;
                VLOG(4) << "max : " << maxValue;


                //cast to double so we can handle integer values in gradient
                uint32_t bucketSize = std::ceil((double)(maxValue - minValue) / NUM_HISTOGRAM_BINS);

                double histSum = 0;
                double totalSum = 0;
                for(auto k = 0; k < NUM_HISTOGRAM_BINS; k++ ){
                    auto binsCount = bucketSize;
                    if(k == NUM_HISTOGRAM_BINS - 1){
                        binsCount = bins.size() - k * bucketSize;
                    }

                    double bucketValue = 0;
                    for(auto i = 0; i < binsCount; i++) {
                        auto binValue = bins[k * bucketSize + i];
                        bucketValue += binValue;
                        totalSum += (k * bucketSize + i) * binValue;
                    }
                    hist[k]= bucketValue;
                    histSum += bucketValue;
                }

                assert(std::accumulate(hist.begin(), hist.end(),0) == histSum);

                //normalize the histogram so that sum(histData)=1;
                for(uint32_t k = 0; k < NUM_HISTOGRAM_BINS; k++) {
                    hist[k] /= histSum;
                }

                //TODO we dont need this info. remove both
                assert(totalSum == sumPixelIntensity);

//                printArray<double>("histogram normalized",&hist[0],20,50);

                //Get peak mode locations
                std::vector<double> modes = {0}; //temp array to store histogram value. Default values to 0.
                std::vector<uint32_t> modesIdx = {0}; //index of histogram value
                modes.resize(NUM_HISTOGRAM_MODES);
                modesIdx.resize(NUM_HISTOGRAM_MODES);
                //sort the NUM_HISTOGRAM_MODES higher values
                for(uint32_t k = 0; k < NUM_HISTOGRAM_BINS; k++) {
                    for(uint32_t l = 0; l < NUM_HISTOGRAM_MODES; l++) {
                        if (hist[k] > modes[l]) {
                            modes[l] = hist[k];
                            modesIdx[l] = k;
                            for (uint32_t m = l; m < NUM_HISTOGRAM_MODES-1; m++) {
                                if (modes[m] > modes[m + 1]) {
                                    auto val = modes[m+1];
                                    auto index = modesIdx[m+1];
                                    modes[m+1] = modes[m];
                                    modes[m] = val;
                                    modesIdx[m+1]=modesIdx[m];
                                    modesIdx[m] = index;
                                }
                                else{
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }


                //find the approximate mode location (average of all values)
                uint32_t sumModes = 0;
                for(uint32_t l = 0; l < NUM_HISTOGRAM_MODES; l++) {
                    sumModes += modesIdx[l];
                    VLOG(4) << "peak value " << l << " : " << modes[l];
                    VLOG(4) << "peak index " << l << " : " << modesIdx[l];
                }
                auto modeLoc = (uint32_t)std::round((double)sumModes / NUM_HISTOGRAM_MODES);
                VLOG(4) << "mode loc : " << modeLoc << " of " << NUM_HISTOGRAM_BINS << "buckets.";

                //lower bound
                uint64_t lowerBound = 3 * (modeLoc + 1) - 1 < NUM_HISTOGRAM_BINS ? 3 * (modeLoc + 1) - 1 : NUM_HISTOGRAM_BINS - 1;

                //upper bound
                uint64_t upperBound = 18 * (modeLoc + 1) - 1  < NUM_HISTOGRAM_BINS ? 18 * (modeLoc + 1) - 1  : NUM_HISTOGRAM_BINS -1;
                auto cutoff = hist[modeLoc] * 0.05; //95% dropoff from the mode location
                for(uint32_t k = modeLoc; k < NUM_HISTOGRAM_BINS; k++) {
                    if(hist[k] <= cutoff && hist[k] > upperBound){
                        upperBound = (uint64_t)hist[k];
                        break;
                    }
                }
                VLOG(1) << "lower bound : " << (uint32_t)lowerBound;
                VLOG(1) << "upper bound : " << (uint32_t)upperBound;


                // compute the area under the histogram between lower and upperBound
                double area = 0;
                for (auto k = lowerBound; k <= upperBound; k++) {
                    if(hist[k] > 0) {
                        area += hist[k];
                    }
                }

                //TODO CHECK calculations are made in float so this value varies slightly, leading to some variability in the threshold chosen.
                //Known ways to overcome the problem?
                VLOG(4) << "area : " << area;

                assert((double)0.0 <= area );
                assert(area < (double)1.0);

                //compute percentile threshold from the empirical model : Y = aX + b
                double saturation1 = 3;
                double saturation2 = 42;
                double a = (95.0 - 40.0) / (saturation1 - saturation2);
                double b = 95 - a * saturation1;
                double percentileThreshold = std::round(a * (area * 100) + b);
                percentileThreshold = (percentileThreshold > 98) ? 98 : percentileThreshold;
                percentileThreshold = (percentileThreshold < 25) ? 25 : percentileThreshold;


                //greedy param taken into account
                assert(-50 <= greedy <= 50);
                percentileThreshold += std::round(greedy);
                percentileThreshold = (percentileThreshold > 100) ? 100 : percentileThreshold;
                percentileThreshold = (percentileThreshold < 0) ? 0 : percentileThreshold;

                VLOG(3) << "percentile of pixels threshold value : " << percentileThreshold;

                double totalPixelThreshold = histSum * ( (double)percentileThreshold / 100);

                //TODO we need to do the sum of bins[k] * k until == percentilePixelThreshold * bins.size()
                auto val = bins.size() - 1;
                auto sumPixels = 0;
                for(auto k = 0; k < bins.size(); k++){
                    if(sumPixels >= totalPixelThreshold){
                        val = k;
                        break;
                    }
                    sumPixels += bins[k];
                }

                T threshold = val;


                VLOG(3) << "threshold pixel gradient intensity value : " << (T)threshold;

                this->addResult(new Threshold<T>(threshold));

                hist.clear();
                bins.clear();
            }


        }

        htgs::ITask <ConvOutMemoryData<T>, Threshold<T>> *copy() override {
            return new FastThresholdFinder(this->getNumThreads(), sampleSize, nbOfSamples,  imageDepth);
        }

        std::string getName() override { return "Threshold Finder"; }


    private:

        void binSample(std::shared_ptr<ConvOutMemoryData<T>> data){
            auto tile = data->getOutputdata()->get();
            auto sampleSize = data->getTileWidth() * data->getTileHeight();
            for(auto k = 0; k < sampleSize; k++ ){
                if(tile[k] != 0){
                    //we also compute min max while at it
                    minValue = tile[k] < minValue ? tile[k] : minValue;
                    maxValue = tile[k] > maxValue ? tile[k] : maxValue;
                    bins[tile[k]]++;
                    sumPixelIntensity += tile[k];
                }
            }
        }

        uint32_t nbOfSamples{}, sampleSize{};

        T minValue = std::numeric_limits<T>::max(),
                maxValue = std::numeric_limits<T>::min();

        double sumPixelIntensity = 0;

        uint32_t greedy = 0; //TODO add to constructor
        ImageDepth imageDepth = ImageDepth::_8U;

        uint32_t counter = 0;

        const uint32_t NUM_HISTOGRAM_MODES = 3;
        const uint32_t NUM_HISTOGRAM_BINS = 1000;

        std::vector<double> hist = {};
        std::vector<double> bins = {};

    };
}

#endif //NEWEGT_FASTTHRESHOLDFINDER_H
