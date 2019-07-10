//
// Created by gerardin on 5/7/19.
//

#ifndef EGT_THRESHOLD_H
#define EGT_THRESHOLD_H

#include <htgs/api/IData.hpp>


namespace egt {

    template <class T>
    class Threshold : public htgs::IData {

        public:
            Threshold(T value) : value(value) {}

        T getValue() const {
            return value;
        }

    private:

            T value;
    };
}



#endif //EGT_THRESHOLD_H
