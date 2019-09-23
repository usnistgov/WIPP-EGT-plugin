//
// Created by gerardin on 8/12/19.
//

#ifndef NEWEGT_NOTRANSFORM_H
#define NEWEGT_NOTRANSFORM_H


#include <FastImage/api/View.h>
#include <htgs/api/ITask.hpp>
#include <egt/data/ConvOutData.h>
#include <glog/logging.h>
#include <egt/utils/Utils.h>
#include <egt/api/DataTypes.h>
#include <egt/memory/ReleaseMemoryRule.h>
#include <egt/data/ConvOutMemoryData.h>
#include <egt/data/GradientView.h>

namespace egt {

    template <class T>
    class NoTransform : public htgs::ITask<htgs::MemoryData<fi::View<T>>, GradientView<T>> {


    private:

        ImageDepth depth = ImageDepth::_8U;
        uint32_t startRow = 1; //row at which the convolution starts
        uint32_t startCol = 1; //col at which the convolution starts

        std::string outputPath = "/home/gerardin/CLionProjects/newEgt/outputs/debug/";
//        std::string outputPath = "/Users/gerardin/Documents/projects/wipp++/egt/outputs/";

    public:

        NoTransform(size_t numThreads, ImageDepth depth, uint32_t startRow, uint32_t startCol) : htgs::ITask<htgs::MemoryData<fi::View<T>>, GradientView<T>> (numThreads), depth(depth), startRow(startRow), startCol(startCol) {}

        /// \brief Do the convolution on a view
        /// \param data View
        void executeTask(std::shared_ptr< htgs::MemoryData<fi::View<T>>> data) override {

            auto view = data->get();
            auto viewWidth = view->getViewWidth();
            auto viewHeight = view->getViewHeight();
            auto viewData = view->getData();

            auto radius = view->getRadius();
            assert(radius >= startRow && radius >= startCol);
            auto tileWidth = view->getTileWidth();
            auto tileHeight = view->getTileHeight();

            VLOG(3) << "No Transform for tile (" << view->getRow() << " , " << view->getCol() << ") ..." ;

            //copy the original view
            T* original = new T[tileWidth * tileHeight]();
            for(auto row = 0; row < view->getTileHeight(); row++){
                std::copy_n(viewData + (row + radius) * tileWidth + radius, tileWidth, original + row * tileWidth);
            }

            auto gradientView = new GradientView<T>(data,original);

            // Write the output tile
            this->addResult(gradientView);
        }

        htgs::ITask <htgs::MemoryData<fi::View<T>>,  GradientView<T>> *copy() override {
            return new NoTransform(this->getNumThreads(), this->depth, this->startRow, this->startCol);
        }

        std::string getName() override { return "No Tranform"; }

    };

}




#endif //NEWEGT_NOTRANSFORM_H
