//
// Created by gerardin on 7/19/19.
//

#ifndef NEWEGT_VIEWANALYZEFILTER_H
#define NEWEGT_VIEWANALYZEFILTER_H

#include <egt/FeatureCollection/Data/ViewOrViewAnalyse.h>
#include "BlobMerger.h"

using namespace fc;

namespace egt {




    template <class T>
    class ViewAnalyseFilter : public htgs::ITask<ViewOrViewAnalyse<T>, ViewAnalyse> {

    public:

        ViewAnalyseFilter(size_t numThreads): ITask<ViewOrViewAnalyse<T>, ViewAnalyse>(numThreads) {};

        void executeTask(std::shared_ptr<ViewOrViewAnalyse<T>> data) override {
            this->addResult(data->getViewAnalyse());
        }

        ITask<ViewOrViewAnalyse<T>, ViewAnalyse> *copy() override {
            return new ViewAnalyseFilter(this->getNumThreads());
        }

        std::string getName() override {
            return "ViewAnalyseFilter";
        }
    };

}

#endif //NEWEGT_VIEWANALYZEFILTER_H
