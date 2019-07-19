//
// Created by gerardin on 5/7/19.
//

#ifndef EGT_THRESHOLDFINDER_H
#define EGT_THRESHOLDFINDER_H

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
    class ThresholdFinder : public htgs::ITask<ConvOutMemoryData<T>, Threshold<T>> {


    public:
        ThresholdFinder(uint32_t width, uint32_t height, uint32_t numTileRow, uint32_t numTileCol, ImageDepth imageDepth) : htgs::ITask<ConvOutMemoryData<T>, Threshold<T>>(1),
                                                                                           imageWidth(width),
                                                                                           imageHeight(height),
                                                                                           numTileRow(numTileRow),
                                                                                           numTileCol(numTileCol),
                                                                                           imageDepth(imageDepth)
                                                                                           {
            this->totalTiles = numTileRow * numTileCol;
            gradient = new T[width * height]();
            hist.reserve(NUM_HISTOGRAM_BINS + 1);
                                                                                           }


        void executeTask(std::shared_ptr<ConvOutMemoryData<T>> data) override {

            copyTile(data);
            data->getOutputdata()->releaseMemory();

            counter++;

            if(counter == totalTiles) {

                VLOG(3) << "Determining Threshold..." ;

                VLOG(4) << "we have run gradient on : " << totalTiles << " tiles.";

                T minValue = std::numeric_limits<T>::max() , maxValue = std::numeric_limits<T>::min();

                auto nonZeroGradient = std::vector<T>();

                //create an histogram of NUM_HISTOGRAM_BINS bins
                //we also collect none zero values as we need them for the final step when applying the percentileThreshold.
                for(auto k = 0; k < imageWidth * imageHeight; k++ ){
                    if(gradient[k] != 0.0) {
                        auto value = gradient[k];
                        auto row = k / imageWidth;
                        auto col = k % imageWidth;

                        minValue = gradient[k] < minValue ? gradient[k] : minValue;
                        maxValue = gradient[k] > maxValue ? gradient[k] : maxValue;
                        nonZeroGradient.push_back(gradient[k]);
                    }
                }

                delete[] gradient;

                VLOG(4) << "Nb of gradient pixels : " << imageWidth * imageHeight;
                VLOG(4) << "Nb of gradient pixels with value of 0 : " << imageWidth * imageHeight - nonZeroGradient.size();

                VLOG(4) << "min : " << minValue;
                VLOG(4) << "max : " << maxValue;


                //cast to double so we can handle integer values in gradient
                double rescale = (double)NUM_HISTOGRAM_BINS / (maxValue - minValue);
                double sum = 0;
                for(auto k = 0; k < nonZeroGradient.size(); k++ ){
                        //we round to closest integer
                        //WORKS ONLY IF VALUE ARE ALL POSITIVES. We are working with gradient magnitude so this works.
//                        auto index = (uint32_t)((gradient[k] - minValue) * rescale + 0.5);
                        auto index = (uint32_t)((nonZeroGradient[k] - minValue) * rescale + 0.5);

                        assert(index >= 0 && index < NUM_HISTOGRAM_BINS + 1);

                        hist[index]++;
                        sum++;
                }

                //normalize the histogram so that sum(histData)=1;
                for(uint32_t k = 0; k < NUM_HISTOGRAM_BINS; k++) {
                    hist[k] /= sum;
                }

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
                //TODO CHECK - taken from the book - in java code we have other values
                double s1 = 3;
                double s2 = 50;
                double a = -1.3517;
                double b = 98.8726;
                double percentileThreshold = std::round(a * (area * 100) + b);
                percentileThreshold = (percentileThreshold > 95) ? 95 : percentileThreshold;
                percentileThreshold = (percentileThreshold < 25) ? 25 : percentileThreshold;


                //greedy param taken into account
                assert(-50 <= greedy <= 50);
                percentileThreshold += std::round(greedy);
                percentileThreshold = (percentileThreshold > 100) ? 100 : percentileThreshold;
                percentileThreshold = (percentileThreshold < 0) ? 0 : percentileThreshold;

                VLOG(3) << "percentile of pixels threshold value : " << percentileThreshold;

                //find the threshold pixel value.
                //we get all non zero pixels and sort them in ascending order, the percentilePixelThreshold'th pixel has the
                //intensity we will use for thresholding.
                //TODO this is slow.
                VLOG(3) << "sorting all pixel values... ";
                sort(nonZeroGradient.begin(), nonZeroGradient.end());

                auto percentilePixelThreshold = percentileThreshold / 100 * nonZeroGradient.size();

                assert(0 <= percentilePixelThreshold <= nonZeroGradient.size());

                T threshold = nonZeroGradient[percentilePixelThreshold];

                VLOG(3) << "threshold pixel gradient intensity value : " << (T)threshold;

                this->addResult(new Threshold<T>(threshold));

                hist.clear();

                nonZeroGradient.clear();
            }


        }

        htgs::ITask <ConvOutMemoryData<T>, Threshold<T>> *copy() override {
            return new ThresholdFinder(imageWidth, imageHeight, numTileRow, numTileCol, imageDepth);
        }

        std::string getName() override { return "Threshold Finder"; }


    private:

        void copyTile(std::shared_ptr<ConvOutMemoryData<T>> data){
            auto tile = data->getOutputdata()->get();
            auto row = data->getGlobalRow(),
                 col = data->getGlobalCol();
            auto
                 tileHeight = data->getTileHeight(),
                 tileWidth = data->getTileWidth();
                 for(auto tileRow = 0 ; tileRow < tileHeight; tileRow++) {
                     assert((row + tileRow) * imageWidth + col <= imageWidth * imageHeight);
                     assert((row + tileRow) * imageWidth + col >= 0);
                     std::copy_n(tile + tileRow * tileWidth, tileWidth, gradient + (row + tileRow) * imageWidth + col);
                 }
        }

        uint32_t imageWidth;
        uint32_t imageHeight;
        uint32_t numTileRow;
        uint32_t numTileCol;
        uint32_t totalTiles;
        uint32_t greedy = 0; //TODO add to constructor
        ImageDepth imageDepth = ImageDepth::_8U;


        T* gradient;

        uint32_t counter = 0;

        const uint32_t NUM_HISTOGRAM_MODES = 3;
        const uint32_t NUM_HISTOGRAM_BINS = 1000;

        std::vector<double> hist = {};

    };
}

#endif //EGT_THRESHOLDFINDER_H
