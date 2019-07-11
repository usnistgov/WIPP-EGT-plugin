//
// Created by gerardin on 7/11/19.
//

#ifndef NEWEGT_VIEWTOCONVOUTDATACONVERTER_H
#define NEWEGT_VIEWTOCONVOUTDATACONVERTER_H

#include <FastImage/api/View.h>
#include <htgs/api/ITask.hpp>
#include <egt/data/ConvOutData.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>
#include <opencv/cv.hpp>
#include <egt/api/DataTypes.h>
#include <egt/data/ThresholdOrView.h>
#include <egt/utils/Utils.h>

/**
 * WORKS ONLY IF RADIUS = 0 ! SINCE WE ARE ONLY RETURNING THE UNDERLYING DATA FROM THE VIEW, THE VIEW MUST BE EQUIVALENT
 * TO THE TILE.
 */

namespace egt {

    template<class T>
    class ViewToConvOutDataConverter : public htgs::ITask<htgs::MemoryData<fi::View<T>>, ConvOutData<T>> {

    public:

        ViewToConvOutDataConverter(size_t numThreads)
                : htgs::ITask<htgs::MemoryData<fi::View<T>>, ConvOutData<T>>(numThreads) {}


        void executeTask(std::shared_ptr<htgs::MemoryData<fi::View<T>>> data) override {
            auto view = data->get();
            auto rawData = view->getData();
            VLOG(2) << "tile (" << view->getRow() << " , " << view->getCol() << ")" ;
            VLOG(2) << "tileWidth : " << view->getTileWidth();
            VLOG(2) << "tileHeight : " << view->getTileHeight();
            VLOG(2) << "viewWidth : " << view->getViewWidth();
            VLOG(2) << "viewHeight : " << view->getViewHeight();

            //the array stored in the view is of fixed size depending on the file metadata tileWidth and tileHeight.
            //we need to copy relevant data back to our smaller array (without the extra padding).

            auto rawDataWidth = view->getViewWidth();

            int32_t tileWidth = view->getTileWidth();
            int32_t tileHeight = view->getTileHeight();
            auto tileOut = new T[tileWidth * tileHeight]();


            for(auto tileRow = 0; tileRow < tileHeight; tileRow++){
                std::copy_n(rawData + tileRow * rawDataWidth, tileWidth , tileOut + tileRow * tileWidth );
            }

            this->addResult(new ConvOutData<T>(tileOut, view->getGlobalYOffset(), view->getGlobalXOffset(), tileWidth, tileHeight));

            // Release the view
            data->releaseMemory();
        }

        htgs::ITask<htgs::MemoryData<fi::View<T>>, ConvOutData<T>> *copy() override {
            return new ViewToConvOutDataConverter(this->getNumThreads());
        }


    };
}


#endif //NEWEGT_VIEWTOCONVOUTDATACONVERTER_H
