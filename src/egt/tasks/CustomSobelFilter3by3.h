//
// Created by gerardin on 5/6/19.
//

#ifndef EGT_SOBELFILTER_H
#define EGT_SOBELFILTER_H

#include <FastImage/api/View.h>
#include <htgs/api/ITask.hpp>
#include <egt/data/ConvOutData.h>
#include <glog/logging.h>
#include <egt/utils/Utils.h>
#include <egt/api/DataTypes.h>

namespace egt {

    template <class T>
    class CustomSobelFilter3by3 : public htgs::ITask<htgs::MemoryData<fi::View<T>>, ConvOutData<T>> {


    private:

        ImageDepth depth = ImageDepth::_8U;
        uint32_t counter = 0;

        std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";

    public:

        CustomSobelFilter3by3(size_t numThreads, ImageDepth depth) : htgs::ITask<htgs::MemoryData<fi::View<T>>,ConvOutData<T>> (numThreads), depth(depth) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {

            VLOG(2) << "Sobel Filter...";

            counter++;

            auto view = data->get();
            auto viewWidth = view->getViewWidth();
            auto viewHeight = view->getViewHeight();



//            printArray<T>("view",view->getData(),viewWidth,viewHeight);

            int32_t
                    radius = view->getRadius();


            float
                    pixelOut = 0,
                    pixelOutX = 0,
                    pixelOutY = 0;

            auto tileWidth = view->getTileWidth();
            auto tileHeight = view->getTileHeight();
            auto *tileOut = new T[tileWidth * tileHeight]();

            auto ksize = 2 * radius + 1;

            assert(radius == 1 );

//            // Iterate through each tile pixel
//            for (auto row = 0; row < viewHeight; ++row) {
//                for (auto col = 0; col < viewWidth; ++col) {
//                    //convolution for one pixel
//                    VLOG(1) << "(" << row <<"," << col << ")";
//
//                            auto p1 = view->getPixel(row - 1, col - 1);
//                            auto p2 = view->getPixel(row -1, col);
//                            auto p3 = view->getPixel(row -1, col + 1);
//                            auto p4 = view->getPixel(row, col-1);
//                            auto p5 = view->getPixel(row, col);
//                            auto p6 = view->getPixel(row, col+1);
//                            auto p7 = view->getPixel(row+1, col-1);
//                            auto p8 = view->getPixel(row+1, col);
//                            auto p9 = view->getPixel(row+1,col+1);
//
//                            auto sum1 = p1 + 2*p2 + p3 - p7 - 2*p8 - p9;
//                            auto sum2 = p1  + 2*p4 + p7 - p3 - 2*p6 - p9;
//                            auto sum = sqrt(sum1*sum1 + sum2*sum2);
//
//                            tileOut[row * tileWidth + col] = sum;
//                }
//            }

            //Emulate Sobel as implemented in ImageJ
            //[description](https://imagejdocu.tudor.lu/faq/technical/what_is_the_algorithm_used_in_find_edges)
            for (auto row = 1; row < tileHeight; ++row) {
                for (auto col = 1; col < tileWidth; ++col) {

                    auto data = view->getData();
                    auto p1 = data[(row - 1)*viewWidth + (col - 1)];
                    auto p2 = data[(row - 1)*viewWidth + (col)];
                    auto p3 =  data[(row - 1)*viewWidth + (col + 1)];
                    auto p4 =  data[(row)*viewWidth + (col -1)];
                    auto p6 =  data[(row)*viewWidth + (col +1)];
                    auto p7 =  data[(row +1)*viewWidth + (col - 1)];
                    auto p8 = data[(row + 1)*viewWidth + (col)];
                    auto p9 =  data[(row + 1)*viewWidth + (col + 1)];

                    auto sum1 = p1 + 2*p2 + p3 - p7 - 2*p8 - p9;
                    auto sum2 = p1  + 2*p4 + p7 - p3 - 2*p6 - p9;
                    auto sum = sqrt(sum1*sum1 + sum2*sum2);

                    tileOut[row * tileWidth + col] = sum;
                }
            }

            auto img5 = cv::Mat(tileHeight, tileWidth, CV_32F, tileOut);
            cv::imwrite(outputPath + "tileoutcustom" + std::to_string(counter)  + ".png" , img5);

          //  printArray<T>("gradient",tileOut,tileWidth,tileHeight);

            // Write the output tile
            this->addResult(new ConvOutData<T>(tileOut, view->getGlobalYOffset(), view->getGlobalXOffset(), tileWidth, tileHeight));
            // Release the view
            data->releaseMemory();
        }

        htgs::ITask <htgs::MemoryData<fi::View<T>>, ConvOutData<T>> *copy() override {
            return new CustomSobelFilter3by3(this->getNumThreads(), this->depth);
        }

        std::string getName() override { return "Custom Sobel Filter 3 * 3"; }

    };

}

#endif //EGT_SOBELFILTER_H
