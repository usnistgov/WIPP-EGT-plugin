//
// Created by gerardin on 7/19/19.
//

#ifndef NEWEGT_SEGMENTATIONOPTIONS_H
#define NEWEGT_SEGMENTATIONOPTIONS_H

#include <cstdint>
#include <egt/api/DataTypes.h>

namespace egt {

    class SegmentationOptions {

    public:

        SegmentationOptions() = default;

        uint32_t MIN_HOLE_SIZE{};
        uint32_t MAX_HOLE_SIZE{};
        uint32_t MIN_OBJECT_SIZE{};

        bool MASK_ONLY = false;

        uint32_t MIN_PIXEL_INTENSITY_PERCENTILE = 0;
        uint32_t MAX_PIXEL_INTENSITY_PERCENTILE = 100;

        JoinOperator KEEP_HOLES_WITH_JOIN_OPERATOR = JoinOperator::AND;

        bool disableIntensityFilter = false;

    };

}

#endif //NEWEGT_SEGMENTATIONOPTIONS_H
