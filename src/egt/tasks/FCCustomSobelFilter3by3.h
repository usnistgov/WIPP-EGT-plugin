//
// Created by gerardin on 7/17/19.
//

#ifndef NEWEGT_FCCUSTOMSOBELFILTER3BY3_H
#define NEWEGT_FCCUSTOMSOBELFILTER3BY3_H


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
    class FCCustomSobelFilter3by3 : public htgs::ITask<htgs::MemoryData<fi::View<T>>,  htgs::MemoryData<fi::View<T>>> {


    private:

        ImageDepth depth = ImageDepth::_8U;
        uint32_t startRow = 1; //row at which the convolution starts
        uint32_t startCol = 1; //col at which the convolution starts

        std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/";
//        std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

    public:

        FCCustomSobelFilter3by3(size_t numThreads, ImageDepth depth, uint32_t startRow, uint32_t startCol) : htgs::ITask<htgs::MemoryData<fi::View<T>>, htgs::MemoryData<fi::View<T>>> (numThreads), depth(depth), startRow(startRow), startCol(startCol) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr< htgs::MemoryData<fi::View<T>>> data) override {

            auto view = data->get();
            auto viewWidth = view->getViewWidth();
            auto viewHeight = view->getViewHeight();
            auto viewData = view->getData();

            auto img4 = cv::Mat(viewHeight, viewWidth, convertToOpencvType(depth), (T*)viewData);
            cv::imwrite(outputPath + "tileout" + std::to_string(view->getRow()) + "-" + std::to_string(view->getCol())  + ".png" , img4);
            img4.release();

//            printArray<T>("view",view->getData(),viewWidth,viewHeight);

            auto radius = view->getRadius();
            assert(radius >= startRow && radius >= startCol);
            auto tileWidth = view->getTileWidth();
            auto tileHeight = view->getTileHeight();

            VLOG(3) << "Custom Sobel Filter for tile (" << view->getRow() << " , " << view->getCol() << ") ..." ;


            T* tileOut = new T[viewWidth * viewHeight]();

            //Emulate Sobel as implemented in ImageJ
            //[description](https://imagejdocu.tudor.lu/faq/technical/what_is_the_algorithm_used_in_find_edges)
            for (auto row = startRow; row < tileHeight + 2 * radius - startRow; ++row) {
                for (auto col = startCol; col < tileWidth + 2 * radius - startCol; ++col) {
                    auto p1 = viewData[(row-1) * viewWidth + (col-1)];
                    auto p2 = viewData[(row-1) * viewWidth + (col)];
                    auto p3 =  viewData[(row-1) * viewWidth + (col+1)];
                    auto p4 =  viewData[(row) * viewWidth + (col - 1)];
                    auto p6 =  viewData[(row) * viewWidth + (col+1)];
                    auto p7 =  viewData[(row+1) * viewWidth + (col-1)];
                    auto p8 = viewData[(row+1) * viewWidth + (col)];
                    auto p9 =  viewData[(row+1) * viewWidth + (col+1)];

                    auto sum1 = p1 + 2*p2 + p3 - p7 - 2*p8 - p9;
                    auto sum2 = p1  + 2*p4 + p7 - p3 - 2*p6 - p9;
                    auto sum = sqrt(sum1*sum1 + sum2*sum2);

                    auto index = row * viewWidth + col;

                    tileOut[index] = (T)sum;

                }
            }

            std::copy_n(tileOut, viewHeight * viewWidth, view->getData());

//            for(auto row = startRow; row < tileHeight + startRow; ++row){
//                std::copy_n(tileOut + row * tileWidth, tileWidth, view->getData() + (startRow + row ) * viewWidth + startCol);
//            }

// FOR DEBUGGING
            auto img5 = cv::Mat(viewHeight, viewWidth, convertToOpencvType(depth), tileOut);
            cv::imwrite(outputPath + "tileoutcustom" + std::to_string(view->getRow()) + "-" + std::to_string(view->getCol())  + ".png" , img5);
            img5.release();

            delete[] tileOut;

            // Write the output tile
            this->addResult(data);
        }

        htgs::ITask <htgs::MemoryData<fi::View<T>>,  htgs::MemoryData<fi::View<T>>> *copy() override {
            return new FCCustomSobelFilter3by3(this->getNumThreads(), this->depth, this->startRow, this->startCol);
        }

        std::string getName() override { return "Custom Sobel Filter 3 * 3"; }

    };

}


#endif //NEWEGT_FCCUSTOMSOBELFILTER3BY3_H
