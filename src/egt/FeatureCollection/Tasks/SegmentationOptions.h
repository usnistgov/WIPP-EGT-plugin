//
// Created by gerardin on 7/17/19.
//

#ifndef NEWEGT_SEGMENTATIONOPTIONS_H
#define NEWEGT_SEGMENTATIONOPTIONS_H

#include <cstdint>

class SegmentationOptions {

public:

    uint32_t getMinHoleSize() const {
        return MIN_HOLE_SIZE;
    }

    void setMinHoleSize(uint32_t MIN_HOLE_SIZE) {
        SegmentationOptions::MIN_HOLE_SIZE = MIN_HOLE_SIZE;
    }

    uint32_t getMinObjectSize() const {
        return MIN_OBJECT_SIZE;
    }

    void setMinObjectSize(uint32_t MIN_OBJECT_SIZE) {
        SegmentationOptions::MIN_OBJECT_SIZE = MIN_OBJECT_SIZE;
    }

    /// \brief Print state
    /// \param os Output stream
    /// \param options SegmentationOptions to print
    /// \return Output stream with the options information
    friend std::ostream &operator<<(std::ostream &os, const SegmentationOptions &options) {
        os << "Segmentation Options: " << std::endl;
        os << "    MIN_HOLE_SIZE: " << options.MIN_HOLE_SIZE << std::endl;
        os << "    MIN_OBJECT_SIZE: " << options.MIN_OBJECT_SIZE << std::endl;
        return os;
    }

private:
    uint32_t MIN_HOLE_SIZE;
    uint32_t MIN_OBJECT_SIZE;
};

#endif //NEWEGT_SEGMENTATIONOPTIONS_H
