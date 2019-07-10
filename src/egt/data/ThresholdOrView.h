//
// Created by Gerardin, Antoine D. (Assoc) on 2019-05-13.
//

#ifndef EGT_THRESHOLDORVIEW_H
#define EGT_THRESHOLDORVIEW_H

#include <FastImage/api/View.h>
#include <htgs/api/MemoryData.hpp>
#include "Threshold.h"

namespace egt {

    template <class T>
    class ThresholdOrView : htgs::IData {

        public:

        explicit ThresholdOrView(Threshold<T> *threshold) : threshold(threshold) {}

        explicit ThresholdOrView(htgs::MemoryData<fi::View<T>> *view) : view(view) {}

        Threshold<T>* getThreshold() {
            return threshold;
        }

        htgs::MemoryData<fi::View<T>>* getView() {
            return view;
        }

    private:
            Threshold<T>* threshold = nullptr;
            htgs::MemoryData<fi::View<T>>* view = nullptr;

    };

}

#endif //EGT_THRESHOLDORVIEW_H
