//
// Created by gerardin on 8/1/19.
//

#ifndef NEWEGT_LISTFEATURES_H
#define NEWEGT_LISTFEATURES_H

#include <htgs/api/IData.hpp>
#include <list>
#include "Feature.h"

namespace egt {
/// \namespace fc FeatureCollection namespace

/**
 * @struct ListBlobs ListBlobs.h <FastImage/FeatureCollection/Data/ListBlobs.h>
 *
 * @brief List of Feature. It needs to inherit from IData to be passed around htgs graph.
 */
struct ListFeatures : public htgs::IData {

    public:

        std::list<Feature *>
                _features{};

        ~ListFeatures() override {
            while (!_features.empty()) delete _features.front(), _features.pop_front();
        }
    };
}


#endif //NEWEGT_LISTFEATURES_H
