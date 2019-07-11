//
// Created by gerardin on 5/7/19.
//

#ifndef EGT_THRESHOLDFINDER_H
#define EGT_THRESHOLDFINDER_H

#include <egt/data/Threshold.h>
#include <egt/data/ConvOutData.h>
#include <htgs/api/ITask.hpp>
#include <algorithm>
#include <glog/logging.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>
#include <opencv/cv.hpp>
#include <egt/utils/Utils.h>

namespace egt {

    template <class T>
    class ThresholdFinder : public htgs::ITask<ConvOutData<T>, Threshold<T>> {


    public:
        ThresholdFinder(uint32_t width, uint32_t height, uint32_t numTileRow, uint32_t numTileCol) : htgs::ITask<ConvOutData<T>, Threshold<T>>(1),
                                                                                           imageWidth(width),
                                                                                           imageHeight(height),
                                                                                           numTileRow(numTileRow),
                                                                                           numTileCol(numTileCol)
                                                                                           {
            this->totalTiles = numTileRow * numTileCol;
            gradient = new T[width * height]();
            hist.resize(NUM_HISTOGRAM_BINS);
                                                                                           }


        void executeTask(std::shared_ptr<ConvOutData<T>> data) override {

            //TODO NO NEED TO COPY data. Just accumulate. Or maybe build the histogram on the fly.
            copyTile(data);
            counter++;

            if(counter == totalTiles) {
                VLOG(1) << "we are done producing the " << totalTiles << " tiles.";


//                printArray<T>("full gradient",gradient,imageWidth,imageHeight);


                std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
//                std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

                cv::Mat image(imageHeight, imageWidth, CV_32F, gradient);
                cv::imwrite(outputPath + "fullGradient.tif", image);
                image.release();

                //TODO MINVALUE IS ALWAYS > 0 AFTER SOBEL? WHAT IS FLOAT? DO WE HAVE TO EXHAUSTIVELY SEARCH?
                // SAMPLING SHOULD BE GOOD ENOUGH? ANOTHER APPROACH IS TO GET IT FROM THE SOBEL OPERATION
                //How to find min and max value for T?
                T minValue = std::numeric_limits<T>::max() , maxValue = std::numeric_limits<T>::min();

                auto nonZeroGradient = std::vector<T>();

                //create an histogram of NUM_HISTOGRAM_BINS bins
                //we also collect none zero values as we need them for the final step when applying the percentileThreshold.
                for(auto k = 0; k < imageWidth * imageHeight; k++ ){
                    if(gradient[k] != 0){
                        minValue = gradient[k] < minValue ? gradient[k] : minValue;
                        maxValue = gradient[k] > maxValue ? gradient[k] : maxValue;
                        nonZeroGradient.push_back(gradient[k]);
                    }
                }



                //TODO [CHECK]
//                //WORKS ONLY IF VALUE ARE ALL POSITIVES. We are working with gradient magnitude so this works.
//                //also we use NUM_HISTOGRAM_BINS - 1 so values are bined in the [0,999] range.
//                //TODO DO WE NEED TO WORK WITH 0 VALUES? OR CAN WE USE DIRECTLY NON-ZEROS GRADIENTS?
//                double rescale = (NUM_HISTOGRAM_BINS - 1) / (maxValue - minValue);
//                double sum = 0;
//                for(auto k = 0; k < imageWidth * imageHeight; k++ ){
//                    //TODO see. here test non-zeros!
//                    if(gradient[k] != 0){
//                        auto index = (uint32_t)((gradient[k] - minValue) * rescale);
//                        hist[index]++;
//                        sum++;
//                    }
//                }


                //TODO [CHECK]  to match previous implementation
                double rescale = NUM_HISTOGRAM_BINS / (maxValue - minValue);
                double sum = 0;
                for(auto k = 0; k < imageWidth * imageHeight; k++ ){
                    //TODO see. here test non-zeros!
                    if(gradient[k] != 0){
                        auto index = (uint32_t)((gradient[k] - minValue) * rescale + 0.5);
                        hist[index]++;
                        sum++;
                    }
                }

                //TODO FOR DEBUG
                float* histAsRawArray = &hist[0];
                printArray<float>("histogram",histAsRawArray,20,50);

                //TODO [CHECK] in the book it is described as before modloc but not in original code
//                //normalize the histogram so that sum(histData)=1;
//                for(uint32_t k = 0; k < NUM_HISTOGRAM_BINS; k++) {
//                    hist[k] /= sum;
//                }

//                printArray<float>("histogram normalized",&hist[0],20,50);

                //Get peak mode locations
                std::vector<float> modes = {0}; //temp array to store histogram value. Default values to 0.
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
                auto sumModes = 0;
                for(uint32_t l = 0; l < NUM_HISTOGRAM_MODES; l++) {
                    sumModes += modesIdx[l];
                    VLOG(1) << "peak value " << l << " : " << modes[l];
                    VLOG(1) << "peak index " << l << " : " << modesIdx[l];
                }
                uint32_t modeLoc = sumModes / NUM_HISTOGRAM_MODES;

                VLOG(1) << "mode loc : " << modeLoc << " of " << NUM_HISTOGRAM_BINS << "buckets.";


                //TODO [CHECK] in previous implementation, it is here
                //normalize the histogram so that sum(histData)=1;
                for(uint32_t k = 0; k < NUM_HISTOGRAM_BINS; k++) {
                hist[k] /= sum;
                }

                //TODO [CHECK] in previous implementation, lower and upper bound have some index conversion
//                //lower bound
//                auto lowerBound = 3 * modeLoc < NUM_HISTOGRAM_BINS ? 3 * modeLoc : NUM_HISTOGRAM_BINS - 1;
//
//                //upper bound
//                auto upperBound = 18 * modeLoc < NUM_HISTOGRAM_BINS ? 18 * modeLoc : NUM_HISTOGRAM_BINS -1;
//                auto cutoff = hist[modeLoc] * 0.05; //95% dropoff from the mode location
//                for(uint32_t k = modeLoc; k < NUM_HISTOGRAM_BINS; k++) {
//                    if(hist[k] <= cutoff && hist[k] > upperBound){
//                        upperBound = hist[k];
//                        break;
//                    }
//                }

                //TODO [CHECK] here we match previous implementation
                //lower bound
                auto lowerBound = 3 * (modeLoc + 1) - 1 < NUM_HISTOGRAM_BINS ? 3 * (modeLoc + 1) - 1 : NUM_HISTOGRAM_BINS - 1;

                //upper bound
                auto upperBound = 18 * (modeLoc + 1) - 1  < NUM_HISTOGRAM_BINS ? 18 * (modeLoc + 1) - 1  : NUM_HISTOGRAM_BINS -1;
                auto cutoff = hist[modeLoc] * 0.05; //95% dropoff from the mode location
                for(uint32_t k = modeLoc; k < NUM_HISTOGRAM_BINS; k++) {
                    if(hist[k] <= cutoff && hist[k] > upperBound){
                        upperBound = hist[k];
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
                        VLOG(1) << "add" << hist[k] << " to area = " << area;
                    }
                }

                assert((double)0.0 < area );
                assert(area < (double)1.0);

                //compute percentile threshold from the empirical model : Y = aX + b
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

                VLOG(1) << "percentile of pixels threshold value : " << (int)percentileThreshold;

                //find the threshold pixel value.
                //we get all non zero pixels and sort them in ascending order, the percentilePixelThreshold'th pixel has the
                //intensity we will use for thresholding.
                sort(nonZeroGradient.begin(), nonZeroGradient.end());
                auto percentilePixelThreshold = percentileThreshold / 100 * nonZeroGradient.size();

                assert(0 <= percentilePixelThreshold <= nonZeroGradient.size());

                T threshold = nonZeroGradient[percentilePixelThreshold];

                VLOG(1) << "threshold pixel gradient intensity value : " << (T)threshold;

                this->addResult(new Threshold<T>(threshold));

                uint32_t count;
                //TODO remove for debug only
                // apply the threshold to the gradient pixels (not just the nonzero ones)
                for (int k = 0; k < imageWidth * imageHeight; k++) {
                    if (gradient[k] >= threshold) {
                        count++;
                    }
                    gradient[k] = (gradient[k] >= threshold) ? 255 : 0;

                }

                egt::printArray<T>("mask",gradient,imageWidth,imageHeight);

                VLOG(1) << " number of pixel in foreground : " << count;

                cv::Mat image2(imageHeight, imageWidth, CV_32F, gradient);
                cv::imwrite(outputPath + "mask.png", image2);
                image.release();


            }


        }

        htgs::ITask <ConvOutData<T>, Threshold<T>> *copy() override {
            return new ThresholdFinder(imageWidth, imageHeight, numTileRow, numTileCol);
        }

        std::string getName() override { return "Threshold Finder"; }


    private:

        void copyTile(std::shared_ptr<ConvOutData<T>> data){
            auto tile = data->getOutputdata();
            auto row = data->getGlobalRow(),
                 col = data->getGlobalCol(),
                 tileHeight = data->getTileHeight(),
                 tileWidth = data->getTileWidth();
                 for(auto tileRow = 0 ; tileRow < tileHeight; tileRow++) {
                     assert((row + tileRow) * imageWidth + col <= imageWidth * imageHeight);
                     assert((row + tileRow) * imageWidth + col >= 0);
                     std::copy_n(tile + tileRow * tileWidth, tileWidth, gradient + (row + tileRow) * imageWidth + col);
                 }
        }

        T* gradient;
        uint32_t imageWidth;
        uint32_t imageHeight;
        uint32_t numTileRow;
        uint32_t numTileCol;
        uint32_t totalTiles;
        uint32_t greedy = 0; //TODO add to constructor

        uint32_t counter = 0;

        const uint32_t NUM_HISTOGRAM_MODES = 3;
        const uint32_t NUM_HISTOGRAM_BINS = 1000; // TODO add to constructor param
        std::vector<float> hist = {};

    };
}

#endif //EGT_THRESHOLDFINDER_H
