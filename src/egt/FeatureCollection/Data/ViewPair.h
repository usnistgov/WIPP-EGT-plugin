//
// Created by gerardin on 8/7/19.
//

#ifndef NEWEGT_VIEWPAIR_H
#define NEWEGT_VIEWPAIR_H

#include <htgs/api/IData.hpp>
#include <FastImage/api/View.h>
#include <htgs/api/MemoryData.hpp>

namespace egt {

    template <class T>
    class ViewPair : htgs::IData {


    public:
        ViewPair(const htgs::MemoryData<fi::View<T>> &originalView, const htgs::MemoryData<fi::View<T>> &gradientView)
                : originalView(originalView), gradientView(gradientView) {}


    private :
        htgs::MemoryData<fi::View<T>> originalView;
        htgs::MemoryData<fi::View<T>> gradientView;
    };
}

#endif //NEWEGT_VIEWPAIR_H
