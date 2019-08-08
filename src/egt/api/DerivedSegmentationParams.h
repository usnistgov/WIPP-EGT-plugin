//
// Created by gerardin on 8/8/19.
//

#ifndef NEWEGT_DERIVEDSEGMENTATIONPARAMS_H
#define NEWEGT_DERIVEDSEGMENTATIONPARAMS_H

#include <limits>

template <class T>
class DerivedSegmentationParams {

public:

    DerivedSegmentationParams() = default;

    T threshold = std::numeric_limits<T>::min(),
      minPixelIntensityValue = std::numeric_limits<T>::min(),
      maxPixelIntensityValue = std::numeric_limits<T>::max();

};

#endif //NEWEGT_DERIVEDSEGMENTATIONPARAMS_H
