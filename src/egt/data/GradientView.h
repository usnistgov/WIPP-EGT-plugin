//
// Created by gerardin on 8/12/19.
//

#ifndef NEWEGT_GRADIENTVIEW_H
#define NEWEGT_GRADIENTVIEW_H

#include <FastImage/api/View.h>
#include <htgs/api/IData.hpp>
#include <htgs/api/MemoryData.hpp>

namespace egt {

    template <class T>
    class GradientView : htgs::IData {

    public :

        GradientView(const std::shared_ptr<htgs::MemoryData<fi::View<T>>> &gradientView, T*originalView) : gradientView(gradientView),
                                                                                          originalView(originalView) {}

        const std::shared_ptr<MemoryData<fi::View<T>>> &getGradientView() const {
            return gradientView;
        }

        T *getOriginalView() const {
            return originalView;
        }

        void releaseMemory(){
            delete[] originalView;
            gradientView->releaseMemory();

    }


    private:

        std::shared_ptr<htgs::MemoryData<fi::View<T>>> gradientView = nullptr;
        T * originalView;


    };
}

#endif //NEWEGT_GRADIENTVIEW_H
