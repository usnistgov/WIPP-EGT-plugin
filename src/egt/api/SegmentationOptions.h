//
// Created by gerardin on 7/19/19.
//

#ifndef NEWEGT_SEGMENTATIONOPTIONS_H
#define NEWEGT_SEGMENTATIONOPTIONS_H

#include <cstdint>

class SegmentationOptions {

public:

    SegmentationOptions() = default;

    uint32_t MIN_HOLE_SIZE{};
    uint32_t MAX_HOLE_SIZE{};
    uint32_t MIN_OBJECT_SIZE{};

    bool MASK_ONLY = false;
};

#endif //NEWEGT_SEGMENTATIONOPTIONS_H
