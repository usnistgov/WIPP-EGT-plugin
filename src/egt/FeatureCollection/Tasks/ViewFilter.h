//
// Created by gerardin on 7/22/19.
//

#ifndef NEWEGT_VIEWFILTER_H
#define NEWEGT_VIEWFILTER_H


#include <egt/FeatureCollection/Data/ViewOrViewAnalyse.h>
#include "BlobMerger.h"

using namespace fc;

namespace egt {

    template <class T>
    class ViewFilter : public htgs::ITask<ViewOrViewAnalyse<T>, htgs::MemoryData<fi::View<T>>> {

    public:

        explicit ViewFilter(size_t numThreads): ITask<ViewOrViewAnalyse<T>, htgs::MemoryData<fi::View<T>>>(numThreads) {};

        void executeTask(std::shared_ptr<ViewOrViewAnalyse<T>> data) override {
            this->addResult(data->getView());
        }

        ITask<ViewOrViewAnalyse<T>, htgs::MemoryData<fi::View<T>>> *copy() override {
            return new ViewFilter(this->getNumThreads());
        }
    };

}

#endif //NEWEGT_VIEWFILTER_H
