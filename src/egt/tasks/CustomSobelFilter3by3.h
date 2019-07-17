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
#include <egt/memory/ReleaseMemoryRule.h>
#include <egt/data/ConvOutMemoryData.h>

namespace egt {

    template <class T>
    class CustomSobelFilter3by3 : public htgs::ITask<htgs::MemoryData<fi::View<T>>, ConvOutMemoryData<T>> {


    private:

        ImageDepth depth = ImageDepth::_8U;
        uint32_t startRow = 1; //row at which the convolution starts
        uint32_t startCol = 1; //col at which the convolution starts

        std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
//        std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

    public:

        CustomSobelFilter3by3(size_t numThreads, ImageDepth depth, uint32_t startRow, uint32_t startCol) : htgs::ITask<htgs::MemoryData<fi::View<T>>,ConvOutMemoryData<T>> (numThreads), depth(depth), startRow(startRow), startCol(startCol) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {

            auto view = data->get();
            auto viewWidth = view->getViewWidth();
            auto viewHeight = view->getViewHeight();

//            printArray<T>("view",view->getData(),viewWidth,viewHeight);

            auto radius = view->getRadius();
            assert(radius >= startRow && radius >= startCol);
            auto tileWidth = view->getTileWidth();
            auto tileHeight = view->getTileHeight();

            VLOG(3) << "Custom Sobel Filter for tile (" << view->getRow() << " , " << view->getCol() << ") ..." ;


            auto tileMemoryData = this-> template getDynamicMemory<T>("gradientTile", new ReleaseMemoryRule(1), (size_t)(tileWidth * tileHeight));
            T* tileOut = tileMemoryData->get();

            //Emulate Sobel as implemented in ImageJ
            //[description](https://imagejdocu.tudor.lu/faq/technical/what_is_the_algorithm_used_in_find_edges)
            for (auto row = startRow; row < tileHeight + startRow; ++row) {
                for (auto col = startCol; col < tileWidth + startCol; ++col) {

                    auto viewData = view->getData();
                    auto p1 = viewData[(row - 1)*viewWidth + (col - 1)];
                    auto p2 = viewData[(row - 1)*viewWidth + (col)];
                    auto p3 =  viewData[(row - 1)*viewWidth + (col + 1)];
                    auto p4 =  viewData[(row)*viewWidth + (col -1)];
                    auto p6 =  viewData[(row)*viewWidth + (col +1)];
                    auto p7 =  viewData[(row +1)*viewWidth + (col - 1)];
                    auto p8 = viewData[(row + 1)*viewWidth + (col)];
                    auto p9 =  viewData[(row + 1)*viewWidth + (col + 1)];

                    auto sum1 = p1 + 2*p2 + p3 - p7 - 2*p8 - p9;
                    auto sum2 = p1  + 2*p4 + p7 - p3 - 2*p6 - p9;
                    auto sum = sqrt(sum1*sum1 + sum2*sum2);

                    auto index = (row - startCol) * tileWidth + (col - startCol);

                    assert(index < tileWidth * tileHeight);
                    assert(index >= 0);

                    tileOut[index] = (T)sum;

                }
            }

// FOR DEBUGGING
//            auto img5 = cv::Mat(tileHeight, tileWidth, convertToOpencvType(depth), tileOut);
//            cv::imwrite(outputPath + "tileoutcustom" + std::to_string(view->getRow()) + "-" + std::to_string(view->getCol())  + ".tiff" , img5);

            // Release the view
            data->releaseMemory();

            // Write the output tile
            this->addResult(new ConvOutMemoryData<T>(tileMemoryData, view->getGlobalYOffset(), view->getGlobalXOffset(), tileWidth, tileHeight));

        }

        htgs::ITask <htgs::MemoryData<fi::View<T>>, ConvOutMemoryData<T>> *copy() override {
            return new CustomSobelFilter3by3(this->getNumThreads(), this->depth, this->startRow, this->startCol);
        }

        std::string getName() override { return "Custom Sobel Filter 3 * 3"; }

    };

}

#endif //EGT_SOBELFILTER_H
